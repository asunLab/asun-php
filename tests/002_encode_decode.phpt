--TEST--
Test ASON encode and decode basics
--SKIPIF--
<?php if (!extension_loaded("ason")) print "skip"; ?>
--FILE--
<?php
$data = ['id' => 1, 'name' => 'Test', 'active' => true];
$encoded = ason_encode($data);
echo $encoded . "\n";
$decoded = ason_decode($encoded);
var_dump($decoded);
?>
--EXPECT--
{id,name,active}:(1,Test,true)
array(3) {
  ["id"]=>
  int(1)
  ["name"]=>
  string(4) "Test"
  ["active"]=>
  bool(true)
}
