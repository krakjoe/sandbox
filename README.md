sandbox
=======
*A possibly dangerous extension, for possibly dangerous individuals!*

[![Build Status](https://travis-ci.org/krakjoe/sandbox.svg?branch=master)](https://travis-ci.org/krakjoe/sandbox)
[![Coverage Status](https://coveralls.io/repos/github/krakjoe/sandbox/badge.svg?branch=master)](https://coveralls.io/github/krakjoe/sandbox?branch=master)

runkit used to contain a sandbox that used TSRM to run PHP code in isolation. PHP7 still has TSRM, however, since it uses honest to goodness thread local storage, and in addition, caches TSRM storage locally in each binary (one in php, one in runkit.so and so on), the same hacks are not possible.

In PHP7 we must create the actual thread for the sandbox, but this extension does not give you userland threading: While we must use a thread for the sandbox, we do not execute any (user) code in parallel. We dispatch Closures to a thread and block while they execute.

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
	* @param entry point for sandbox
	* @param array arguments for closure
	* @throws Error if $closure or $argv are not valid
	**/
	public function enter(Closure $closure, array $argv = []) : mixed;

	/*
	* Shall close the sandbox
	* @throws Error if the sandbox is unusable
	*/
	public function close();
}
```

Sandboxing
==========

By it's nature, a sandbox is an isolated environment; Things may go very badly wrong in the sandbox environment and not effect the environment that created it. This means that we must try very hard to limit the influence each environment has on the other. So the prototype and instructions of entry point ```Closures``` are verified to ensure they will not reduce or break isolation.

In practice this means that entry point closures must not:

  * accept or return by reference
  * accept or return non-scalar values (array, object)
  * execute a limited set of instructions

Instructions prohibited directly in the sandbox are:

  * declare (anonymous) function
  * declare (anonymous) class
  * lexical scope access

Nothing is prohibited in the files which the sandbox may include, but allowing these actions directly in the code which the sandbox executes at entry would break the isolation of the sandbox such that we couldn't be sure the system would remain stable.

Features
========

Note that, there is hardly any API, and many features (compared to the runkit sandbox) are missing ...

Pull requests are welcome, and if issues appear, I'll try to fix them ...
