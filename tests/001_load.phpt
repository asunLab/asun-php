--TEST--
Check if ason is loaded
--SKIPIF--
<?php if (!extension_loaded("ason")) print "skip"; ?>
--FILE--
<?php
echo "The extension " . (extension_loaded("ason") ? "is loaded" : "is not loaded");
?>
--EXPECT--
The extension is loaded
