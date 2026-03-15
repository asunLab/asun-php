// ============================================================================
// ASON PHP Extension — High-performance SIMD-accelerated ASON codec
// Direct Zend API, zero intermediate layer, zero-copy where possible
// ============================================================================

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern "C" {
#include "php.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
}

#include "ason_core.h"
#include <string>
#include <cstring>
#include <functional>
#include <vector>

using namespace ason_core;

// ============================================================================
// PHP → ASON Encoder (zval → string, zero intermediate structs)
// ============================================================================

static void encode_zval(std::string& buf, zval* val);
static void encode_zval_typed(std::string& buf, zval* val);
static bool is_sequential_array(HashTable* ht);
static void append_schema_type(std::string& buf, zval* val);

static void encode_array_schema(std::string& buf, zval* val, bool typed) {
    HashTable* ht = Z_ARRVAL_P(val);
    zend_string* key;
    zend_ulong idx;
    bool first = true;
    ZEND_HASH_FOREACH_KEY(ht, idx, key) {
        if (!first) buf.push_back(',');
        first = false;
        if (key) {
            append_schema_name(buf, ZSTR_VAL(key), ZSTR_LEN(key));
        } else {
            char numbuf[32];
            int n = std::snprintf(numbuf, sizeof(numbuf), "%lu", idx);
            append_schema_name(buf, numbuf, (size_t)n);
        }
        if (typed) {
            zval* v = key ? zend_hash_find(ht, key) : zend_hash_index_find(ht, idx);
            if (v) {
                buf.push_back('@');
                append_schema_type(buf, v);
            }
        }
    } ZEND_HASH_FOREACH_END();
}

static bool is_sequential_array(HashTable* ht) {
    zend_ulong expected = 0;
    zend_ulong idx;
    zend_string* key;
    ZEND_HASH_FOREACH_KEY(ht, idx, key) {
        if (key || idx != expected++) return false;
    } ZEND_HASH_FOREACH_END();
    return true;
}

static void append_schema_type(std::string& buf, zval* val) {
    switch (Z_TYPE_P(val)) {
        case IS_LONG:
            buf.append("int");
            break;
        case IS_DOUBLE:
            buf.append("float");
            break;
        case IS_TRUE:
        case IS_FALSE:
            buf.append("bool");
            break;
        case IS_STRING:
            buf.append("str");
            break;
        case IS_NULL:
            buf.append("str?");
            break;
        case IS_ARRAY: {
            HashTable* inner = Z_ARRVAL_P(val);
            if (!is_sequential_array(inner)) {
                buf.push_back('{');
                encode_array_schema(buf, val, true);
                buf.push_back('}');
                break;
            }

            zval* first_el = zend_hash_index_find(inner, 0);
            buf.push_back('[');
            if (!first_el) {
                buf.append("str");
            } else {
                append_schema_type(buf, first_el);
            }
            buf.push_back(']');
            break;
        }
        default:
            buf.append("str");
            break;
    }
}

static void encode_tuple_values(std::string& buf, zval* val) {
    HashTable* ht = Z_ARRVAL_P(val);
    zval* v;
    bool first = true;
    ZEND_HASH_FOREACH_VAL(ht, v) {
        if (!first) buf.push_back(',');
        first = false;
        encode_zval(buf, v);
    } ZEND_HASH_FOREACH_END();
}

static void encode_zval(std::string& buf, zval* val) {
    switch (Z_TYPE_P(val)) {
        case IS_LONG: append_i64(buf, Z_LVAL_P(val)); break;
        case IS_DOUBLE: append_f64(buf, Z_DVAL_P(val)); break;
        case IS_TRUE: buf.append("true", 4); break;
        case IS_FALSE: buf.append("false", 5); break;
        case IS_STRING: append_str(buf, Z_STRVAL_P(val), Z_STRLEN_P(val)); break;
        case IS_NULL: break; // empty
        case IS_ARRAY: {
            HashTable* ht = Z_ARRVAL_P(val);
            if (is_sequential_array(ht)) {
                zval* first_el = zend_hash_index_find(ht, 0);
                if (first_el && Z_TYPE_P(first_el) == IS_ARRAY && !is_sequential_array(Z_ARRVAL_P(first_el))) {
                    // Nested object arrays are body-only: [(...),(...)]
                    buf.push_back('[');
                    bool f2 = true;
                    zval* el;
                    ZEND_HASH_FOREACH_VAL(ht, el) {
                        if (!f2) buf.push_back(',');
                        f2 = false;
                        buf.push_back('(');
                        encode_tuple_values(buf, el);
                        buf.push_back(')');
                    } ZEND_HASH_FOREACH_END();
                    buf.push_back(']');
                } else {
                    // Simple array: [v1,v2,...]
                    buf.push_back('[');
                    bool f2 = true;
                    zval* el;
                    ZEND_HASH_FOREACH_VAL(ht, el) {
                        if (!f2) buf.push_back(',');
                        f2 = false;
                        encode_zval(buf, el);
                    } ZEND_HASH_FOREACH_END();
                    buf.push_back(']');
                }
            } else {
                // Nested structs are body-only tuples: (v1,v2,...)
                buf.push_back('(');
                encode_tuple_values(buf, val);
                buf.push_back(')');
            }
            break;
        }
        default: break;
    }
}

// ============================================================================
// ASON → PHP Decoder (string → zval, direct Zend construction)
// ============================================================================

static void decode_value_to_zval(const char*& p, const char* e, zval* rv);

static void decode_array_values(const char*& p, const char* e, const Schema& schema, zval* rv) {
    array_init(rv);
    int parsed = 0;
    for (int i = 0; i < schema.count; i++) {
        skip_ws_comments(p, e);
        if (p < e && *p == ')') break;
        if (i > 0) {
            if (*p == ',') { p++; skip_ws_comments(p, e); if (p < e && *p == ')') break; }
            else if (*p == ')') break;
        }
        zval field_val;
        if (at_value_end(p, e)) {
            ZVAL_NULL(&field_val);
        } else {
            decode_value_to_zval(p, e, &field_val);
        }
        zend_string* key = zend_string_init(schema.fields[i].data(), schema.fields[i].size(), 0);
        zend_hash_add_new(Z_ARRVAL_P(rv), key, &field_val);
        zend_string_release(key);
        parsed++;
    }
    
    // Pad remaining missing fields with NULL
    for (int i = parsed; i < schema.count; i++) {
        zval field_val;
        ZVAL_NULL(&field_val);
        zend_string* key = zend_string_init(schema.fields[i].data(), schema.fields[i].size(), 0);
        zend_hash_add_new(Z_ARRVAL_P(rv), key, &field_val);
        zend_string_release(key);
    }

    // skip remaining
    for (;;) {
        skip_ws_comments(p, e);
        if (p >= e || *p == ')') break;
        if (*p == ',') { p++; skip_ws_comments(p, e); if (p >= e || *p == ')') break; } else break;
        skip_value(p, e);
    }
    skip_ws_comments(p, e);
    if (p < e && *p == ')') p++;
}

static void decode_value_to_zval(const char*& p, const char* e, zval* rv) {
    skip_ws_comments(p, e);
    if (p >= e) { ZVAL_NULL(rv); return; }

    // Array [...]
    if (*p == '[') {
        p++;
        array_init(rv);
        bool first = true;
        for (;;) {
            skip_ws_comments(p, e);
            if (p >= e || *p == ']') { if (p < e) p++; break; }
            if (!first) { if (*p == ',') { p++; skip_ws_comments(p, e); if (p < e && *p == ']') { p++; break; } } }
            first = false;
            zval el;
            decode_value_to_zval(p, e, &el);
            zend_hash_next_index_insert(Z_ARRVAL_P(rv), &el);
        }
        return;
    }

    // Tuple (...) — positional struct without schema
    if (*p == '(') {
        p++;
        array_init(rv);
        bool first = true;
        int idx = 0;
        for (;;) {
            skip_ws_comments(p, e);
            if (p >= e || *p == ')') { if (p < e) p++; break; }
            if (!first) { if (*p == ',') { p++; skip_ws_comments(p, e); if (p < e && *p == ')') { p++; break; } } }
            first = false;
            zval el;
            if (at_value_end(p, e)) { ZVAL_NULL(&el); }
            else decode_value_to_zval(p, e, &el);
            zend_hash_next_index_insert(Z_ARRVAL_P(rv), &el);
            idx++;
        }
        return;
    }

    // Schema-prefixed: {schema}:(...) or [{schema}]:(...),...
    if (*p == '{') {
        Schema schema = parse_schema(p, e);
        skip_ws_comments(p, e);
        if (p < e && *p == ':') {
            p++;
            skip_ws_comments(p, e);
            if (p < e && *p == '(') {
                p++;
                decode_array_values(p, e, schema, rv);
                return;
            }
        }
        ZVAL_NULL(rv);
        return;
    }

    // Try bool
    if (p + 4 <= e && p[0] == 't' && p[1] == 'r' && p[2] == 'u' && p[3] == 'e') {
        if (p + 4 >= e || p[4] == ',' || p[4] == ')' || p[4] == ']' || p[4] == ' ' || p[4] == '\t' || p[4] == '\n') {
            ZVAL_TRUE(rv); p += 4; return;
        }
    }
    if (p + 5 <= e && p[0] == 'f' && p[1] == 'a' && p[2] == 'l' && p[3] == 's' && p[4] == 'e') {
        if (p + 5 >= e || p[5] == ',' || p[5] == ')' || p[5] == ']' || p[5] == ' ' || p[5] == '\t' || p[5] == '\n') {
            ZVAL_FALSE(rv); p += 5; return;
        }
    }

    // Quoted string
    if (*p == '"') {
        std::string s = parse_quoted(p, e);
        ZVAL_STRINGL(rv, s.data(), s.size());
        return;
    }

    // Number or plain string
    const char* start = p;
    bool is_neg = false, has_dot = false, is_num = true;
    if (p < e && *p == '-') { is_neg = true; p++; }
    const char* dstart = p;
    while (p < e && *p >= '0' && *p <= '9') p++;
    if (p < e && *p == '.') { has_dot = true; p++; while (p < e && *p >= '0' && *p <= '9') p++; }
    // Check if it's really a delimiter-terminated number
    if (p > dstart && (p >= e || *p == ',' || *p == ')' || *p == ']' || *p == ' ' || *p == '\t' || *p == '\n')) {
        if (has_dot) {
            double v; std::from_chars(start, p, v);
            ZVAL_DOUBLE(rv, v);
        } else {
            int64_t v = 0; bool neg = (start[0] == '-');
            const char* q = neg ? start + 1 : start;
            while (q < p) { v = v * 10 + (*q - '0'); q++; }
            ZVAL_LONG(rv, neg ? -v : v);
        }
        return;
    }

    // Fall back: it's a plain string value
    p = start;
    std::string s = parse_plain(p, e);
    ZVAL_STRINGL(rv, s.data(), s.size());
}

// Top-level decoder
static void decode_top(const char*& p, const char* e, zval* rv) {
    skip_ws_comments(p, e);
    if (p >= e) { ZVAL_NULL(rv); return; }

    // [{schema}]:(tuple),(tuple),...
    if (*p == '[' && p + 1 < e && p[1] == '{') {
        p++; // skip [
        Schema schema = parse_schema(p, e);
        skip_ws_comments(p, e);
        if (p < e && *p == ']') p++;
        skip_ws_comments(p, e);
        if (p < e && *p == ':') p++;

        array_init(rv);
        for (;;) {
            skip_ws_comments(p, e);
            if (p >= e || *p != '(') break;
            p++;
            zval row;
            decode_array_values(p, e, schema, &row);
            zend_hash_next_index_insert(Z_ARRVAL_P(rv), &row);
            skip_ws_comments(p, e);
            if (p < e && *p == ',') { p++; } else break;
        }
        return;
    }

    // {schema}:(data)
    if (*p == '{') {
        Schema schema = parse_schema(p, e);
        skip_ws_comments(p, e);
        if (p < e && *p == ':') p++;
        skip_ws_comments(p, e);
        if (p < e && *p == '(') {
            p++;
            decode_array_values(p, e, schema, rv);
            return;
        }
        ZVAL_NULL(rv);
        return;
    }

    decode_value_to_zval(p, e, rv);
}

// ============================================================================
// Binary encode/decode (direct zval, no intermediate)
// ============================================================================

static void encode_bin_zval(std::string& buf, zval* val) {
    switch (Z_TYPE_P(val)) {
        case IS_LONG: {
            int64_t v = Z_LVAL_P(val);
            buf.append((const char*)&v, 8);
            break;
        }
        case IS_DOUBLE: {
            double v = Z_DVAL_P(val);
            buf.append((const char*)&v, 8);
            break;
        }
        case IS_TRUE: buf.push_back(1); break;
        case IS_FALSE: buf.push_back(0); break;
        case IS_STRING: {
            uint32_t len = Z_STRLEN_P(val);
            buf.append((const char*)&len, 4);
            buf.append(Z_STRVAL_P(val), len);
            break;
        }
        case IS_NULL: {
            uint8_t marker = 0;
            buf.push_back(marker);
            break;
        }
        case IS_ARRAY: {
            HashTable* ht = Z_ARRVAL_P(val);
            uint32_t cnt = zend_hash_num_elements(ht);
            buf.append((const char*)&cnt, 4);
            zval* v;
            ZEND_HASH_FOREACH_VAL(ht, v) {
                encode_bin_zval(buf, v);
            } ZEND_HASH_FOREACH_END();
            break;
        }
        default: buf.push_back(0); break;
    }
}

static void decode_bin_to_zval(const char*& p, const char* e, zval* rv, zval* type_hint);

static void decode_bin_auto(const char*& p, const char* e, zval* rv) {
    // Without type hint, we cannot decode binary (it's typeless)
    // This is used only for arrays; individual items need type hints
    ZVAL_NULL(rv);
}

static void decode_bin_to_zval(const char*& p, const char* e, zval* rv, zval* type_hint) {
    if (!type_hint || Z_TYPE_P(type_hint) == IS_NULL) {
        ZVAL_NULL(rv);
        return;
    }
    
    // Recursive schema handling for nested arrays/structs
    if (Z_TYPE_P(type_hint) == IS_ARRAY) {
        HashTable* ht = Z_ARRVAL_P(type_hint);
        if (p + 4 > e) { ZVAL_NULL(rv); return; }
        uint32_t cnt; memcpy(&cnt, p, 4); p += 4;  // nested arrays always have count
        
        array_init(rv);
        if (is_sequential_array(ht)) {
            // Nested vector
            zval* inner_hint = zend_hash_index_find(ht, 0);
            for (uint32_t i = 0; i < cnt && p < e; i++) {
                zval el;
                decode_bin_to_zval(p, e, &el, inner_hint);
                zend_hash_next_index_insert(Z_ARRVAL_P(rv), &el);
            }
        } else {
            // Nested struct
            zend_string* key;
            zval* inner_hint;
            ZEND_HASH_FOREACH_STR_KEY_VAL(ht, key, inner_hint) {
                if (!key) continue;
                zval field;
                decode_bin_to_zval(p, e, &field, inner_hint);
                zend_hash_add_new(Z_ARRVAL_P(rv), key, &field);
            } ZEND_HASH_FOREACH_END();
        }
        return;
    }

    if (Z_TYPE_P(type_hint) != IS_STRING) { ZVAL_NULL(rv); return; }
    const char* t = Z_STRVAL_P(type_hint);
    size_t tlen = Z_STRLEN_P(type_hint);

    if (tlen == 3 && memcmp(t, "int", 3) == 0) {
        if (p + 8 > e) { ZVAL_NULL(rv); return; }
        int64_t v; memcpy(&v, p, 8); p += 8;
        ZVAL_LONG(rv, v);
    } else if (tlen == 5 && memcmp(t, "float", 5) == 0) {
        if (p + 8 > e) { ZVAL_NULL(rv); return; }
        double v; memcpy(&v, p, 8); p += 8;
        ZVAL_DOUBLE(rv, v);
    } else if (tlen == 4 && memcmp(t, "bool", 4) == 0) {
        if (p >= e) { ZVAL_NULL(rv); return; }
        ZVAL_BOOL(rv, *p != 0); p++;
    } else if (tlen == 3 && memcmp(t, "str", 3) == 0) {
        if (p + 4 > e) { ZVAL_NULL(rv); return; }
        uint32_t len; memcpy(&len, p, 4); p += 4;
        if (p + len > e) { ZVAL_NULL(rv); return; }
        ZVAL_STRINGL(rv, p, len); p += len;
    } else {
        ZVAL_NULL(rv);
    }
}

// ============================================================================
// Pretty format implementation
// ============================================================================

namespace ason_core {
std::string pretty_format(const std::string& compact) {
    if (compact.empty()) return compact;
    auto mat = build_match_table(compact);
    std::string out;
    out.reserve(compact.size() * 2);
    int pos = 0, depth = 0, n = (int)compact.size();

    // Simplified pretty formatter
    auto write_indent = [&]() { for (int i = 0; i < depth; i++) out.append("  "); };

    std::function<void()> write_group;
    std::function<void(int)> write_element;

    write_element = [&](int boundary) {
        while (pos < boundary && compact[pos] != ',') {
            char ch = compact[pos];
            if (ch == '{' || ch == '(' || ch == '[') write_group();
            else if (ch == '"') {
                out.push_back('"'); pos++;
                while (pos < n) { char c = compact[pos]; out.push_back(c); pos++; if (c == '\\' && pos < n) { out.push_back(compact[pos]); pos++; } else if (c == '"') break; }
            }
            else { out.push_back(ch); pos++; }
        }
    };

    write_group = [&]() {
        if (pos >= n) return;
        char ch = compact[pos];
        if (ch != '{' && ch != '(' && ch != '[') { while (pos < n && compact[pos] != ',' && compact[pos] != ')' && compact[pos] != '}' && compact[pos] != ']') { out.push_back(compact[pos]); pos++; } return; }
        int cl = mat[pos]; if (cl < 0) { out.push_back(ch); pos++; return; }
        int width = cl - pos + 1;
        if (width <= 100) { pretty_inline(out, compact, pos, cl + 1); pos = cl + 1; return; }
        char close_ch = compact[cl];
        out.push_back(ch); pos++;
        if (pos >= cl) { out.push_back(close_ch); pos = cl + 1; return; }
        out.push_back('\n'); depth++;
        bool first = true;
        while (pos < cl) {
            if (compact[pos] == ',') pos++;
            if (!first) { out.push_back(','); out.push_back('\n'); }
            first = false;
            write_indent();
            write_element(cl);
        }
        out.push_back('\n'); depth--;
        write_indent();
        out.push_back(close_ch); pos = cl + 1;
    };

    // Top level
    if (compact[0] == '[' && pos + 1 < n && compact[1] == '{') {
        out.push_back('['); pos++;
        write_group();
        if (pos < n && compact[pos] == ']') { out.push_back(']'); pos++; }
        if (pos < n && compact[pos] == ':') { out.append(":\n"); pos++; }
        depth++;
        bool first = true;
        while (pos < n) {
            // skip whitespace
            while (pos < n && (compact[pos] == ' ' || compact[pos] == '\n' || compact[pos] == '\t')) pos++;
            if (pos >= n) break;
            if (compact[pos] == ',') { pos++; continue; } // skip comma between tuples
            if (compact[pos] != '(' && compact[pos] != '{' && compact[pos] != '[') break;
            if (!first) { out.push_back(','); out.push_back('\n'); }
            first = false;
            write_indent(); write_group();
        }
        out.push_back('\n'); depth--;
    } else if (compact[0] == '{') {
        write_group();
        if (pos < n && compact[pos] == ':') {
            out.push_back(':'); pos++;
            if (pos < n) {
                int cl2 = mat[pos];
                if (cl2 >= 0 && cl2 - pos + 1 <= 100) {
                    pretty_inline(out, compact, pos, cl2 + 1); pos = cl2 + 1;
                } else {
                    out.push_back('\n'); depth++;
                    write_indent(); write_group(); depth--;
                }
            }
        }
    } else {
        out.append(compact.substr(pos));
    }
    return out;
}
} // namespace ason_core

// ============================================================================
// PHP function implementations
// ============================================================================

PHP_FUNCTION(ason_encode) {
    zval* input;
    ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_ZVAL(input) ZEND_PARSE_PARAMETERS_END();
    try {
        std::string buf;
        buf.reserve(256);
        if (Z_TYPE_P(input) == IS_ARRAY && !is_sequential_array(Z_ARRVAL_P(input))) {
            buf.push_back('{');
            encode_array_schema(buf, input, false);
            buf.push_back('}');
            buf.push_back(':');
            buf.push_back('(');
            encode_tuple_values(buf, input);
            buf.push_back(')');
        } else if (Z_TYPE_P(input) == IS_ARRAY) {
            HashTable* ht = Z_ARRVAL_P(input);
            zval* first_el = zend_hash_index_find(ht, 0);
            if (first_el && Z_TYPE_P(first_el) == IS_ARRAY && !is_sequential_array(Z_ARRVAL_P(first_el))) {
                buf.push_back('[');
                buf.push_back('{');
                encode_array_schema(buf, first_el, false);
                buf.push_back('}');
                buf.push_back(']');
                buf.push_back(':');
                bool f2 = true;
                zval* el;
                ZEND_HASH_FOREACH_VAL(ht, el) {
                    if (!f2) buf.push_back(',');
                    f2 = false;
                    buf.push_back('(');
                    encode_tuple_values(buf, el);
                    buf.push_back(')');
                } ZEND_HASH_FOREACH_END();
            } else {
                encode_zval(buf, input);
            }
        } else {
            encode_zval(buf, input);
        }
        RETURN_STRINGL(buf.data(), buf.size());
    } catch (const std::exception& ex) {
        zend_throw_exception(zend_ce_exception, ex.what(), 0);
        RETURN_THROWS();
    }
}

PHP_FUNCTION(ason_decode) {
    char* input; size_t input_len;
    ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_STRING(input, input_len) ZEND_PARSE_PARAMETERS_END();
    try {
        const char* p = input;
        const char* e = input + input_len;
        decode_top(p, e, return_value);
    } catch (const std::exception& ex) {
        zend_throw_exception(zend_ce_exception, ex.what(), 0);
        RETURN_THROWS();
    }
}

PHP_FUNCTION(ason_encodeBinary) {
    zval* input;
    ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_ZVAL(input) ZEND_PARSE_PARAMETERS_END();
    try {
        std::string buf;
        buf.reserve(256);
        if (Z_TYPE_P(input) == IS_ARRAY) {
            HashTable* ht = Z_ARRVAL_P(input);
            if (is_sequential_array(ht)) {
                uint32_t cnt = zend_hash_num_elements(ht);
                buf.append((const char*)&cnt, 4);
                zval* v;
                ZEND_HASH_FOREACH_VAL(ht, v) { encode_bin_zval(buf, v); } ZEND_HASH_FOREACH_END();
            } else {
                zval* v;
                ZEND_HASH_FOREACH_VAL(ht, v) { encode_bin_zval(buf, v); } ZEND_HASH_FOREACH_END();
            }
        } else {
            encode_bin_zval(buf, input);
        }
        RETURN_STRINGL(buf.data(), buf.size());
    } catch (const std::exception& ex) {
        zend_throw_exception(zend_ce_exception, ex.what(), 0);
        RETURN_THROWS();
    }
}

PHP_FUNCTION(ason_decodeBinary) {
    char* input; size_t input_len;
    zval* schema = nullptr;
    ZEND_PARSE_PARAMETERS_START(2, 2) Z_PARAM_STRING(input, input_len) Z_PARAM_ARRAY(schema) ZEND_PARSE_PARAMETERS_END();
    try {
        const char* p = input;
        const char* e = input + input_len;
        HashTable* ht = Z_ARRVAL_P(schema);

        // Check if schema is sequential (array of structs) or assoc (single struct)
        if (is_sequential_array(ht)) {
            // Schema like ['int', 'str', 'bool'] — not really useful
            // Expect array: count + elements
            if (p + 4 > e) { ZVAL_NULL(return_value); return; }
            uint32_t cnt; memcpy(&cnt, p, 4); p += 4;
            array_init(return_value);
            // Each element decoded using first schema type
            zval* type_hint = zend_hash_index_find(ht, 0);
            for (uint32_t i = 0; i < cnt && p < e; i++) {
                zval el;
                decode_bin_to_zval(p, e, &el, type_hint);
                zend_hash_next_index_insert(Z_ARRVAL_P(return_value), &el);
            }
        } else {
            // Assoc schema like ['id' => 'int', 'name' => 'str']
            array_init(return_value);
            zend_string* key;
            zend_ulong idx;
            zval* type_val;
            ZEND_HASH_FOREACH_KEY_VAL(ht, idx, key, type_val) {
                zval field;
                decode_bin_to_zval(p, e, &field, type_val);
                if (key) {
                    zend_hash_add_new(Z_ARRVAL_P(return_value), key, &field);
                } else {
                    char numbuf[32];
                    int n = std::snprintf(numbuf, sizeof(numbuf), "%lu", idx);
                    zend_string* skey = zend_string_init(numbuf, (size_t)n, 0);
                    zend_hash_add_new(Z_ARRVAL_P(return_value), skey, &field);
                    zend_string_release(skey);
                }
            } ZEND_HASH_FOREACH_END();
        }
    } catch (const std::exception& ex) {
        zend_throw_exception(zend_ce_exception, ex.what(), 0);
        RETURN_THROWS();
    }
}

PHP_FUNCTION(ason_encodeTyped) {
    zval* input;
    ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_ZVAL(input) ZEND_PARSE_PARAMETERS_END();
    try {
        std::string buf;
        buf.reserve(256);
        if (Z_TYPE_P(input) == IS_ARRAY && !is_sequential_array(Z_ARRVAL_P(input))) {
            buf.push_back('{');
            encode_array_schema(buf, input, true);
            buf.push_back('}');
            buf.push_back(':');
            buf.push_back('(');
            encode_tuple_values(buf, input);
            buf.push_back(')');
        } else if (Z_TYPE_P(input) == IS_ARRAY) {
            HashTable* ht = Z_ARRVAL_P(input);
            zval* first_el = zend_hash_index_find(ht, 0);
            if (first_el && Z_TYPE_P(first_el) == IS_ARRAY && !is_sequential_array(Z_ARRVAL_P(first_el))) {
                buf.push_back('[');
                buf.push_back('{');
                encode_array_schema(buf, first_el, true);
                buf.push_back('}');
                buf.push_back(']');
                buf.push_back(':');
                bool f2 = true;
                zval* el;
                ZEND_HASH_FOREACH_VAL(ht, el) {
                    if (!f2) buf.push_back(',');
                    f2 = false;
                    buf.push_back('(');
                    encode_tuple_values(buf, el);
                    buf.push_back(')');
                } ZEND_HASH_FOREACH_END();
            } else {
                encode_zval(buf, input);
            }
        } else {
            encode_zval(buf, input);
        }
        RETURN_STRINGL(buf.data(), buf.size());
    } catch (const std::exception& ex) {
        zend_throw_exception(zend_ce_exception, ex.what(), 0);
        RETURN_THROWS();
    }
}

PHP_FUNCTION(ason_encodePretty) {
    zval* input;
    ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_ZVAL(input) ZEND_PARSE_PARAMETERS_END();
    try {
        std::string buf;
        buf.reserve(256);
        if (Z_TYPE_P(input) == IS_ARRAY && !is_sequential_array(Z_ARRVAL_P(input))) {
            buf.push_back('{');
            encode_array_schema(buf, input, false);
            buf.push_back('}');
            buf.push_back(':');
            buf.push_back('(');
            encode_tuple_values(buf, input);
            buf.push_back(')');
        } else if (Z_TYPE_P(input) == IS_ARRAY) {
            HashTable* ht = Z_ARRVAL_P(input);
            zval* first_el = zend_hash_index_find(ht, 0);
            if (first_el && Z_TYPE_P(first_el) == IS_ARRAY && !is_sequential_array(Z_ARRVAL_P(first_el))) {
                buf.push_back('[');
                buf.push_back('{');
                encode_array_schema(buf, first_el, false);
                buf.push_back('}');
                buf.push_back(']');
                buf.push_back(':');
                bool f2 = true;
                zval* el;
                ZEND_HASH_FOREACH_VAL(ht, el) {
                    if (!f2) buf.push_back(',');
                    f2 = false;
                    buf.push_back('(');
                    encode_tuple_values(buf, el);
                    buf.push_back(')');
                } ZEND_HASH_FOREACH_END();
            } else {
                encode_zval(buf, input);
            }
        } else {
            encode_zval(buf, input);
        }
        std::string pretty = pretty_format(buf);
        RETURN_STRINGL(pretty.data(), pretty.size());
    } catch (const std::exception& ex) {
        zend_throw_exception(zend_ce_exception, ex.what(), 0);
        RETURN_THROWS();
    }
}

PHP_FUNCTION(ason_encodePrettyTyped) {
    zval* input;
    ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_ZVAL(input) ZEND_PARSE_PARAMETERS_END();
    try {
        std::string buf;
        buf.reserve(256);
        if (Z_TYPE_P(input) == IS_ARRAY && !is_sequential_array(Z_ARRVAL_P(input))) {
            buf.push_back('{');
            encode_array_schema(buf, input, true);
            buf.push_back('}');
            buf.push_back(':');
            buf.push_back('(');
            encode_tuple_values(buf, input);
            buf.push_back(')');
        } else if (Z_TYPE_P(input) == IS_ARRAY) {
            HashTable* ht = Z_ARRVAL_P(input);
            zval* first_el = zend_hash_index_find(ht, 0);
            if (first_el && Z_TYPE_P(first_el) == IS_ARRAY && !is_sequential_array(Z_ARRVAL_P(first_el))) {
                buf.push_back('[');
                buf.push_back('{');
                encode_array_schema(buf, first_el, true);
                buf.push_back('}');
                buf.push_back(']');
                buf.push_back(':');
                bool f2 = true;
                zval* el;
                ZEND_HASH_FOREACH_VAL(ht, el) {
                    if (!f2) buf.push_back(',');
                    f2 = false;
                    buf.push_back('(');
                    encode_tuple_values(buf, el);
                    buf.push_back(')');
                } ZEND_HASH_FOREACH_END();
            } else {
                encode_zval(buf, input);
            }
        } else {
            encode_zval(buf, input);
        }
        std::string pretty = pretty_format(buf);
        RETURN_STRINGL(pretty.data(), pretty.size());
    } catch (const std::exception& ex) {
        zend_throw_exception(zend_ce_exception, ex.what(), 0);
        RETURN_THROWS();
    }
}

// ============================================================================
// Module definition
// ============================================================================

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_ason_encode, 0, 1, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, data, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_ason_decode, 0, 1, IS_MIXED, 0)
    ZEND_ARG_TYPE_INFO(0, input, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_ason_encodeBinary, 0, 1, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, data, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_ason_decodeBinary, 0, 2, IS_MIXED, 0)
    ZEND_ARG_TYPE_INFO(0, input, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, schema, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_ason_encodeTyped, 0, 1, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, data, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_ason_encodePretty, 0, 1, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, data, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_ason_encodePrettyTyped, 0, 1, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, data, IS_MIXED, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry ason_functions[] = {
    PHP_FE(ason_encode,           arginfo_ason_encode)
    PHP_FE(ason_decode,           arginfo_ason_decode)
    PHP_FE(ason_encodeBinary,     arginfo_ason_encodeBinary)
    PHP_FE(ason_decodeBinary,     arginfo_ason_decodeBinary)
    PHP_FE(ason_encodeTyped,      arginfo_ason_encodeTyped)
    PHP_FE(ason_encodePretty,     arginfo_ason_encodePretty)
    PHP_FE(ason_encodePrettyTyped, arginfo_ason_encodePrettyTyped)
    PHP_FE_END
};

PHP_MINFO_FUNCTION(ason) {
    php_info_print_table_start();
    php_info_print_table_header(2, "ason support", "enabled");
    php_info_print_table_row(2, "Version", "1.0.0");
    php_info_print_table_row(2, "SIMD",
#if defined(ASON_AVX2)
        "AVX2 + SSE2"
#elif defined(ASON_SSE2)
        "SSE2"
#elif defined(ASON_NEON)
        "NEON (ARM64)"
#else
        "Scalar fallback"
#endif
    );
    php_info_print_table_end();
}

zend_module_entry ason_module_entry = {
    STANDARD_MODULE_HEADER,
    "ason",
    ason_functions,
    NULL, NULL, NULL, NULL,
    PHP_MINFO(ason),
    "1.0.0",
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_ASON
extern "C" {
    ZEND_GET_MODULE(ason)
}
#endif
