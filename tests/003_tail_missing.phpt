--TEST--
Test padding tail-missing tuple fields with null
--SKIPIF--
<?php if (!extension_loaded("ason")) print "skip"; ?>
--FILE--
<?php
$decoded = ason_decode('{a,b,c}:(1,)');
var_dump($decoded);
?>
--EXPECT--
array(3) {
  ["a"]=>
  int(1)
  ["b"]=>
  NULL
  ["c"]=>
  NULL
}
