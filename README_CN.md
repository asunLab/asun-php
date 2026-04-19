# asun-php

[![PHP 8.4+](https://img.shields.io/badge/PHP-8.4%2B-blue.svg)](https://www.php.net/)
[![C++ Extension](https://img.shields.io/badge/type-C%2B%2B%20扩展-green.svg)](#)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

高性能 PHP C++ 扩展，用于 [ASUN](https://github.com/asunLab/asun)（Array-Schema Unified Notation）—— SIMD 加速（SSE2/AVX2/NEON），零拷贝解析，直接调用 Zend API，无中间层。

[English](https://github.com/asunLab/asun-php/blob/main/README.md)

## 什么是 ASUN？

ASUN 将**模式**与**数据**分离，消除了 JSON 中的重复键：

```text
JSON (100 tokens):
{"users":[{"id":1,"name":"Alice","active":true},{"id":2,"name":"Bob","active":false}]}

ASUN (~35 tokens, 节省 65%):
[{id@int, name@str, active@bool}]:(1,Alice,true),(2,Bob,false)
```

---

## 为什么选择 ASUN

**json**

标准 JSON 会在每条记录里重复所有字段名。无论是发给 LLM、通过 API 传输，还是服务之间交换数据，这种重复都会浪费 Token、带宽和阅读成本：

```json
[
  { "id": 1, "name": "Alice", "active": true },
  { "id": 2, "name": "Bob", "active": false },
  { "id": 3, "name": "Carol", "active": true }
]
```

**asun**

ASUN 只声明 **一次** Schema，后续每一行只保留值：

```asun
[{id, name, active}]:
  (1,Alice,true),
  (2,Bob,false),
  (3,Carol,true)
```

**这通常意味着更少的 token、更小的体积，更清晰的结构, 以及比重复键名 JSON 更快的解析。**

---

## 当前语法

- `@` 是字段绑定符，例如 `{id@int,name@str}`。
- 对基本类型来说，`@int`、`@str` 之类是可选提示；对复杂字段来说，`@{...}` 和 `@[...]` 是必需的结构绑定。
- header / body 之间仍然使用 `:` 分隔，例如 `{id@int,name@str}:(1,Alice)`。
- 专用 map 语法已经移除。键值集合请改用 entry-object list，例如 `attrs@[{key@str,value@int}]`。

## 性能 (PHP 8.4, x86_64 SSE2/AVX2)

### 序列化 (ASUN 比 json_encode 快 2.7–4.1 倍)

| 场景            | json_encode | asun_encode | 加速比    |
| --------------- | ----------- | ----------- | --------- |
| 扁平结构 × 100  | 3.66 ms     | 0.88 ms     | **4.16x** |
| 扁平结构 × 500  | 15.02 ms    | 4.03 ms     | **3.73x** |
| 扁平结构 × 1000 | 31.89 ms    | 8.66 ms     | **3.68x** |
| 扁平结构 × 5000 | 152.69 ms   | 56.63 ms    | **2.70x** |

### 反序列化 (ASUN 比 json_decode 快 2.2–2.5 倍)

| 场景            | json_decode | asun_decode | 加速比    |
| --------------- | ----------- | ----------- | --------- |
| 扁平结构 × 100  | 7.13 ms     | 2.93 ms     | **2.43x** |
| 扁平结构 × 500  | 35.35 ms    | 16.03 ms    | **2.21x** |
| 扁平结构 × 1000 | 69.06 ms    | 29.83 ms    | **2.32x** |
| 扁平结构 × 5000 | 347.81 ms   | 154.45 ms   | **2.25x** |

### 极深嵌套处理 (5层级嵌套反序列化)

| 场景             | json_decode | asun_decode | 加速比     |
| ---------------- | ----------- | ----------- | ---------- |
| 10 家跨国公司级  | 30.38 ms    | 0.42 ms     | **72.34x** |
| 50 家跨国公司级  | 148.92 ms   | 1.92 ms     | **77.51x** |
| 100 家跨国公司级 | 325.97 ms   | 3.88 ms     | **83.91x** |

### 体积节省 (带 ASUN Binary 压缩版)

| 场景               | JSON      | ASUN 文本 | ASUN 二进制 | 节省比例 (文本/二进) |
| ------------------ | --------- | --------- | ----------- | -------------------- |
| 扁平结构 × 100     | 12,071 B  | 5,614 B   | 7,446 B     | **53% / 38%**        |
| 扁平结构 × 5000    | 612,808 B | 287,851 B | 372,250 B   | **53% / 39%**        |
| 100 跨国公司级单查 | 431,612 B | 231,011 B | 251,830 B   | **46% / 42%**        |

## 特性

- **纯 C++ 扩展** —— 直接调用 Zend API，无中间层，无 FFI 开销
- **SIMD 加速** —— SSE2/AVX2 (x86_64) 和 NEON (ARM64)，带标量回退
- **零拷贝解析** —— 尽可能避免堆分配的字符串扫描
- **PHP 8.4 兼容** —— 适配最新 PHP 版本，完整 arginfo
- **二进制格式** —— 紧凑的二进制序列化，最大化吞吐量
- **美化输出** —— 智能缩进，自动换行
- **IDE 支持** —— 提供 PHP stubs，支持自动补全和类型检查

## 安装

### 从源码编译

```bash
cd asun-php
phpize
./configure --enable-asun
make -j$(nproc)
# 可选：系统级安装
sudo make install
```

### 启用扩展

```ini
; php.ini
extension=asun.so
```

或命令行使用：

```bash
php -d extension=path/to/modules/asun.so your_script.php
```

## API 参考

| 函数                               | 说明                           |
| ---------------------------------- | ------------------------------ |
| `asun_encode($data)`               | 编码为 ASUN 格式               |
| `asun_decode($string)`             | 解码 ASUN 字符串为 PHP 值      |
| `asun_encodeBinary($data)`         | 编码为 ASUN 二进制格式         |
| `asun_decodeBinary($str, $schema)` | 使用类型模式解码二进制         |
| `asun_encodeTyped($data)`          | 编码时在模式中包含基本类型提示 |
| `asun_encodePretty($data)`         | 编码为美化格式                 |
| `asun_encodePrettyTyped($data)`    | 编码为美化格式 + 基本类型提示  |

## 快速开始

```php
<?php

// 序列化结构体（关联数组）
$user = ['id' => 1, 'name' => 'Alice', 'active' => true];
$asun = asun_encode($user);
// → "{id,name,active}:(1,Alice,true)"

// 带基本类型提示
$typed = asun_encodeTyped($user);
// → "{id@int,name@str,active@bool}:(1,Alice,true)"

// 反序列化
$decoded = asun_decode($asun);
// → ['id' => 1, 'name' => 'Alice', 'active' => true]

// 结构体数组 —— 模式只写一次
$users = [
    ['id' => 1, 'name' => 'Alice', 'active' => true],
    ['id' => 2, 'name' => 'Bob', 'active' => false],
];
$vec = asun_encode($users);
// → "[{id,name,active}]:(1,Alice,true),(2,Bob,false)"
```

### 二进制格式

```php
<?php
$user = ['id' => 42, 'name' => 'Alice', 'active' => true];

// 编码为二进制（紧凑、快速）
$bin = asun_encodeBinary($user);
echo strlen($bin); // 18 字节 vs 36 字节文本

// 使用类型模式解码
$decoded = asun_decodeBinary($bin, [
    'id' => 'int',
    'name' => 'str',
    'active' => 'bool',
]);
```

### 美化输出

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

## 运行示例与基准测试

扩展编译完成后，你可以直接在命令行中运行自带的示例和基准测试：

```bash
# 运行基础用法示例
php -d extension=modules/asun.so examples/basic.php

# 运行复杂嵌套结构示例
php -d extension=modules/asun.so examples/complex.php

# 运行性能基准测试 (JSON / ASUN / BIN)
php -d extension=modules/asun.so examples/bench.php
```

## 为什么 ASUN 更快？

1. **零键重复** —— 模式只写一次，数据行仅携带值
2. **直接 Zend API** —— 无中间数据结构，直接构建 PHP `zval` 和 `HashTable`
3. **SIMD 字符串扫描** —— SSE2/AVX2/NEON 向量化字符检测（每周期 16 字节）
4. **快速数字格式化** —— 两位数查找表，浮点数快速路径
5. **模式驱动解析** —— 位置映射 O(1) 访问，无逐行键哈希
6. **最小化分配** —— 栈上模式解析（最多 64 字段，零堆分配）

## 测试套件

基础单元测试覆盖率：

- ✅ 结构体编解码往返
- ✅ 类型模式编码
- ✅ 数组编解码往返
- ✅ 美化格式（单结构体、数组）
- ✅ 二进制编解码往返
- ✅ 嵌套结构体
- ✅ 数组字段
- ✅ 可选/空字段
- ✅ 特殊字符（引号、转义、换行、制表符、反斜杠）
- ✅ 注释解析
- ✅ 多行格式
- ✅ 与 C++ 格式交叉兼容
- ✅ 大数据集（1000+ 条记录）

## IDE 支持

复制 `stubs/asun.php` 到项目中以获得 IDE 自动补全：

```bash
cp stubs/asun.php /path/to/your/project/.stubs/
```

然后在 IDE 中配置 stubs 目录。

## 许可证

MIT
