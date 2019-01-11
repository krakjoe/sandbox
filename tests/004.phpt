--TEST--
Check sandbox closed
--SKIPIF--
<?php
if (!extension_loaded('sandbox')) {
	echo 'skip';
}
?>
--FILE--
<?php
$sandbox = new sandbox\Runtime();

var_dump($sandbox->enter(function() {
	return 10;
}));

$sandbox->close();

try {
	$sandbox->enter(function(){});
} catch (\sandbox\Exception $e) {
	echo "OK\n";
}

try {
	$sandbox->close();
} catch (\sandbox\Exception $e) {
	echo "OK";
}
?>
--EXPECT--
int(10)
OK
OK
