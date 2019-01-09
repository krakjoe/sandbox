--TEST--
ZEND_DECLARE_ANON_CLASS
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
		new class {};
	});
} catch (Throwable $t) {
	var_dump($t->getMessage());
}
?>
--EXPECT--
string(56) "cannot declare anonymous classes directly in the sandbox"


