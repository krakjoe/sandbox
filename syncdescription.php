#!/usr/bin/env php
<?php

$r = fopen(__DIR__ . '/README.md', 'r') or exit(1);
$s = false;
$b = '';
while ($b = fgets($r)) {
	if (trim($b, "\n") == "Requirements") {
		break;
	}
	if (preg_match('/\[.*Status\]/', $b)) {
		$s = true;
		continue;
	}
	if ($s) {
		$b = preg_replace(
			['/^( +)*\*/', '/```/'],
			['*', '"'],
			$b
		);
		$b = wordwrap($b);
		$desc .= $b;
	}
}
$pkg = simplexml_load_file("package.xml") or exit(2);
$pkg->description = trim($desc, "\n");
$pkg->asXML("package.xml");

fclose($r);
echo "package.xml updated, check git diff\n";

