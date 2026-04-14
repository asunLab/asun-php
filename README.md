# asun-php

[![PHP 8.4+](https://img.shields.io/badge/PHP-8.4%2B-blue.svg)](https://www.php.net/)
[![C++ Extension](https://img.shields.io/badge/type-C%2B%2B%20extension-green.svg)](#)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

A high-performance PHP C++ extension for [ASUN](https://github.com/asun-lab/asun) (Array-Schema Unified Notation) — SIMD-accelerated (SSE2/AVX2/NEON), zero-copy parsing, direct Zend API with no intermediate layers.

[中文文档](README_CN.md)

## What is ASUN?

ASUN separates **schema** from **data**, eliminating repetitive keys found in JSON:

```text
JSON (100 tokens):
{"users":[{"id":1,"name":"Alice","active":true},{"id":2,"name":"Bob","active":false}]}

ASUN (~35 tokens, 65% saving):
[{id@int, name@str, active@bool}]:(1,Alice,true),(2,Bob,false)
```

## Current Syntax

- `@` is the field binding marker, for example `{id@int,name@str}`.
- Scalar hints such as `@int` and `@str` are optional; structural bindings such as `@{...}` and `@[...]` are required for complex fields.
- Header/body separation stays `:`, for example `{id@int,name@str}:(1,Alice)`.
- Dedicated map syntax has been removed. Key-value collections should be modeled as entry-object lists such as `attrs@[{key@str,value@int}]`.

## Performance (PHP 8.4, x86_64 SSE2/AVX2)

### Serialization (ASUN is 2.7–4.1x faster than json_encode)

| Scenario           | json_encode | asun_encode | Speedup   |
| ------------------ | ----------- | ----------- | --------- |
| Flat struct × 100  | 3.66 ms     | 0.88 ms     | **4.16x** |
| Flat struct × 500  | 15.02 ms    | 4.03 ms     | **3.73x** |
| Flat struct × 1000 | 31.89 ms    | 8.66 ms     | **3.68x** |
| Flat struct × 5000 | 152.69 ms   | 56.63 ms    | **2.70x** |

### Deserialization (ASUN is 2.2–2.5x faster than json_decode)

| Scenario           | json_decode | asun_decode | Speedup   |
| ------------------ | ----------- | ----------- | --------- |
| Flat struct × 100  | 7.13 ms     | 2.93 ms     | **2.43x** |
| Flat struct × 500  | 35.35 ms    | 16.03 ms    | **2.21x** |
| Flat struct × 1000 | 69.06 ms    | 29.83 ms    | **2.32x** |
| Flat struct × 5000 | 347.81 ms   | 154.45 ms   | **2.25x** |

### Deep Nesting (5-Level Deep)

| Scenario      | json_decode | asun_decode | Speedup    |
| ------------- | ----------- | ----------- | ---------- |
| 10 Companies  | 30.38 ms    | 0.42 ms     | **72.34x** |
| 50 Companies  | 148.92 ms   | 1.92 ms     | **77.51x** |
| 100 Companies | 325.97 ms   | 3.88 ms     | **83.91x** |

### Size Savings

| Scenario           | JSON      | ASUN Text | ASUN Bin  | Saving (Text/Bin) |
| ------------------ | --------- | --------- | --------- | ----------------- |
| Flat struct × 100  | 12,071 B  | 5,614 B   | 7,446 B   | **53% / 38%**     |
| Flat struct × 5000 | 612,808 B | 287,851 B | 372,250 B | **53% / 39%**     |
| 100 Companies      | 431,612 B | 231,011 B | 251,830 B | **46% / 42%**     |

## Features

- **Pure C++ Extension** — direct Zend API, no intermediate layers, no FFI overhead
- **SIMD-accelerated** — SSE2/AVX2 (x86_64) and NEON (ARM64) with scalar fallback
- **Zero-copy parsing** — string scanning without heap allocation where possible
- **PHP 8.4 compatible** — built for the latest PHP with proper arginfo
- **Binary format** — compact binary serialization for maximum throughput
- **Pretty formatting** — smart indentation with configurable width
- **IDE support** — PHP stubs included for autocomplete and type checking

## Installation

### Build from source

```bash
cd asun-php
phpize
./configure --enable-asun
make -j$(nproc)
# Optional: install system-wide
sudo make install
```

### Enable the extension

```ini
; php.ini
extension=asun.so
```

Or use per-command:

```bash
php -d extension=path/to/modules/asun.so your_script.php
```

## API Reference

| Function                           | Description                                       |
| ---------------------------------- | ------------------------------------------------- |
| `asun_encode($data)`               | Encode to ASUN format                             |
| `asun_decode($string)`             | Decode ASUN string to PHP value                   |
| `asun_encodeBinary($data)`         | Encode to ASUN binary format                      |
| `asun_decodeBinary($str, $schema)` | Decode binary with type schema                    |
| `asun_encodeTyped($data)`          | Encode with scalar type hints in schema           |
| `asun_encodePretty($data)`         | Encode with pretty formatting                     |
| `asun_encodePrettyTyped($data)`    | Encode with pretty formatting + scalar type hints |

## Quick Start

```php
<?php

// Serialize a struct (associative array)
$user = ['id' => 1, 'name' => 'Alice', 'active' => true];
$asun = asun_encode($user);
// → "{id,name,active}:(1,Alice,true)"

// With scalar type hints
$typed = asun_encodeTyped($user);
// → "{id@int,name@str,active@bool}:(1,Alice,true)"

// Deserialize
$decoded = asun_decode($asun);
// → ['id' => 1, 'name' => 'Alice', 'active' => true]

// Vec of structs — schema written once
$users = [
    ['id' => 1, 'name' => 'Alice', 'active' => true],
    ['id' => 2, 'name' => 'Bob', 'active' => false],
];
$vec = asun_encode($users);
// → "[{id,name,active}]:(1,Alice,true),(2,Bob,false)"
```

### Binary Format

```php
<?php
$user = ['id' => 42, 'name' => 'Alice', 'active' => true];

// Encode to binary (compact, fast)
$bin = asun_encodeBinary($user);
echo strlen($bin); // 18 bytes vs 36 bytes text

// Decode with type schema
$decoded = asun_decodeBinary($bin, [
    'id' => 'int',
    'name' => 'str',
    'active' => 'bool',
]);
```

### Pretty Format

```php
<?php
$users = [
    ['id' => 1, 'name' => 'Alice', 'active' => true],
    ['id' => 2, 'name' => 'Bob', 'active' => false],
];

echo asun_encodePrettyTyped($users);
// [{id@int, name@str, active@bool}]:
//   (1, Alice, true),
//   (2, Bob, false)
```

## Running Examples & Benchmarks

Once the extension is built, you can run the included examples and benchmarks directly from the command line:

```bash
# Run basic examples
php -d extension=modules/asun.so examples/basic.php

# Run complex nested structure examples
php -d extension=modules/asun.so examples/complex.php

# Run performance benchmarks (JSON / ASUN / BIN)
php -d extension=modules/asun.so examples/bench.php
```

## Why is ASUN Faster?

1. **Zero key repetition** — Schema is written once; data rows carry only values
2. **Direct Zend API** — No intermediate data structures; directly constructs PHP `zval` and `HashTable`
3. **SIMD string scanning** — SSE2/AVX2/NEON vectorized character detection (16 bytes/cycle)
4. **Fast number formatting** — Two-digit lookup table, fast-path float formatting
5. **Schema-driven parsing** — Positional field mapping with O(1) access, no per-row key hashing
6. **Minimal allocation** — Stack-based schema parsing (max 64 fields, zero heap)

## Test Suite

Basic test coverage:

- ✅ Struct encode/decode roundtrip
- ✅ Typed schema encode
- ✅ Vec encode/decode roundtrip
- ✅ Pretty format (single struct, vec)
- ✅ Binary encode/decode roundtrip
- ✅ Nested structs
- ✅ Array fields
- ✅ Optional/null fields
- ✅ Special characters (quotes, escapes, newlines, tabs, backslashes)
- ✅ Comments parsing
- ✅ Multiline format
- ✅ Cross-compatibility with C++ format
- ✅ Large dataset (1000+ records)

## IDE Support

Copy `stubs/asun.php` to your project for IDE autocomplete:

```bash
cp stubs/asun.php /path/to/your/project/.stubs/
```

Then configure your IDE to include the stubs directory for autocompletion.

## License

MIT
