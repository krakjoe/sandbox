--TEST--
Copy try
--SKIPIF--
<?php
if (!extension_loaded('sandbox')) {
	echo 'skip';
}
?>
--FILE--
<?php
$sandbox = new sandbox\Runtime();

var_dump($sandbox->enter(function(){
	try {
		throw Error();
	} catch(Error $ex) {
		echo "OK\n";
	} finally {
		return true;
	}
}));
?>
--EXPECT--
OK
bool(true)

