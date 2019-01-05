sandbox
=======
*A dangerous extension, for dangerous individuals!*

[![Build Status](https://travis-ci.org/krakjoe/sandbox.svg?branch=master)](https://travis-ci.org/krakjoe/sandbox)
[![Coverage Status](https://coveralls.io/repos/github/krakjoe/sandbox/badge.svg?branch=master)](https://coveralls.io/github/krakjoe/sandbox?branch=master)


This is a sandbox for PHP7 ... it's largely untested, quite dangerous, and a little unfinished.

runkit used to contain a sandbox that used TSRM to run PHP code in isolation. PHP7 still has TSRM, however, since it uses honest to goodness thread local storage, and in addition, caches TSRM storage locally in each binary (one in php, one in runkit.so and so on), the same hacks are not possible.

In PHP7 we must create the actual thread for the sandbox, but this extension does not give you userland threading.

Requirements
============

  * PHP 7.1+
  * ZTS
  * <pthread.h>

API
===

```
class sandbox\Runtime {
	/*
	* Shall construct a new sandbox thread
	*/
	public function __construct();
	
	/*
	* Shall enter into the thread at the given entry point
	*/
	public function enter(Closure closure) : mixed;

	/*
	* Shall close the sandbox
	*/
	public function close();
}
```

Features
========

Note that, there is hardly any API, and many features (compared to the runkit sandbox) are missing ...

Pull requests are welcome, and if issues appear, I'll try to fix them ...
