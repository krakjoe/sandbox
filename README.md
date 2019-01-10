sandbox
=======

[![Build Status](https://travis-ci.org/krakjoe/sandbox.svg?branch=master)](https://travis-ci.org/krakjoe/sandbox)
[![Coverage Status](https://coveralls.io/repos/github/krakjoe/sandbox/badge.svg?branch=master)](https://coveralls.io/github/krakjoe/sandbox?branch=master)

Sandboxing
==========

A sandbox is an isolated environment (a thread in our case); Things may go very badly wrong in the sandbox environment and not effect the environment that created it. This means that we must try very hard to limit the influence each environment has on the other. So the prototype and instructions of entry point ```Closures``` are verified to ensure they will not reduce or break isolation.

In practice this means entry point closures must not:

  * accept or return by reference
  * accept or return non-scalar values (array, object)
  * execute a limited set of instructions

Instructions prohibited directly in the sandbox are:

  * declare (anonymous) function
  * declare (anonymous) class
  * lexical scope access

Nothing is prohibited in the files which the sandbox may include, but allowing these actions directly in the code which the sandbox executes at entry would break the isolation of the sandbox such that we couldn't be sure the system would remain stable.

With these restrictions in place, we can be sure that a sandbox may do anything up to but excluding making PHP segfault, and not effect the environment that created it.

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

Features
========

Pull requests are welcome, and if issues appear, I'll try to fix them ...
