--TEST--
sandbox can't create sandbox
--SKIPIF--
<?php
if (!extension_loaded('sandbox')) {
	echo 'skip';
}
?>
--FILE--
<?php
$sandbox = new sandbox\Runtime();

$sandbox->enter(function(){
	try {
		new \sandbox\Runtime();
	} catch (\sandbox\Exception $ex) {
		echo "OK\n";
	}
});
?>
--EXPECT--
OK

