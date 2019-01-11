--TEST--
bailed
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
		throw new Error();
	});
} catch (\sandbox\Exception $ex) {
	echo "OK";
} catch (Error $er) { 
	/* can't catch here what is thrown in sandbox */
}
?>
--EXPECTF--
Fatal error: Uncaught Error in %s:6
Stack trace:
#0 [internal function]: unknown()
#1 {main}
  thrown in %s on line 6
OK

