--TEST--
Copy arguments (OK)
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
	$sandbox->enter(function() {
		var_dump(func_get_args());
	}, [
		1,2,3, "hello"
	]);
} catch (Error $ex) {
	var_dump($ex->getMessage());
}
?>
--EXPECT--
array(4) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
  [3]=>
  string(5) "hello"
}

