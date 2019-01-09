--TEST--
ZEND_DECLARE_INHERITED_CLASS
--SKIPIF--
<?php
if (!extension_loaded('sandbox')) {
	echo 'skip';
}
?>
--FILE--
<?php
$sandbox = new sandbox\Runtime();
try {
	$sandbox->enter(function(){
		class Foo extends Bar {}
	});
} catch (Throwable $t) {
	var_dump($t->getMessage());
}
?>
--EXPECT--
string(46) "cannot declare classes directly in the sandbox"


