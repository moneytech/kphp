@kphp_should_fail
/Function calls on the left-hand side of \?\?= are not supported/
<?php

class A {
    /** @var ?int */
    public $x = null;

    public static function make() : A {
      return new A();
    }
}

A::make()->x ??= 99;
