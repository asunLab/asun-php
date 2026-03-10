--TEST--
Test binary encoding and decoding for array of structs
--SKIPIF--
<?php if (!extension_loaded("ason")) print "skip"; ?>
--FILE--
<?php
$data = [
    ["id" => 1, "name" => "A"],
    ["id" => 2, "name" => "B"]
];
$bin = ason_encodeBinary($data);
$schema = [['id' => 'int', 'name' => 'str']];
$decoded = ason_decodeBinary($bin, $schema);
var_dump($decoded);
?>
--EXPECT--
array(2) {
  [0]=>
  array(2) {
    ["id"]=>
    int(1)
    ["name"]=>
    string(1) "A"
  }
  [1]=>
  array(2) {
    ["id"]=>
    int(2)
    ["name"]=>
    string(1) "B"
  }
}
