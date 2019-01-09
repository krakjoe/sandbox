sandbox
=======
*A possibly dangerous extension, for possibly dangerous individuals!*

[![Build Status](https://travis-ci.org/krakjoe/sandbox.svg?branch=master)](https://travis-ci.org/krakjoe/sandbox)
[![Coverage Status](https://coveralls.io/repos/github/krakjoe/sandbox/badge.svg?branch=master)](https://coveralls.io/github/krakjoe/sandbox?branch=master)

runkit used to contain a sandbox that used TSRM to run PHP code in isolation. PHP7 still has TSRM, however, since it uses honest to goodness thread local storage, and in addition, caches TSRM storage locally in each binary (one in php, one in runkit.so and so on), the same hacks are not possible.

In PHP7 we must create the actual thread for the sandbox, but this extension does not give you userland threading: While we must use a thread for the sandbox, we do not execute any (user) code in parallel. We dispatch Closures to a user thread and block while they execute.

Requirements
============

  * PHP 7.1+
  * ZTS
  * <pthread.h>

API
===

```php
<?php
class sandbox\Runtime {
	/**
	* Shall construct a new sandbox thread
        * @param array optional ini configuration
	*/
	public function __construct(array $ini = []);
	
	/**
	* Shall enter into the thread at the given entry point
	* @param Closure to execute in the sandbox
	* @throws Error if the sandbox is unusable
	* Note: simple scalars may be copied out
	**/
	public function enter(Closure closure) : mixed;

	/*
	* Shall close the sandbox
	* @throws Error if the sandbox is unusable
	*/
	public function close();
}
```

Features
========

Note that, there is hardly any API, and many features (compared to the runkit sandbox) are missing ...

Pull requests are welcome, and if issues appear, I'll try to fix them ...
