@ok
<?php

require_once("polyfills.php");

class A {
    public $nm_a = 10;
    public $t_a;
    public $a_a = [1, 2, 3];
    public $s_a = "asdf";
    public $n_a = NULL;
    public $t_or_false_a = false;

    public function __construct() {
        $this->t_a = tuple(10, "asdf");
        $this->t_or_false_a = tuple("asfd");
    }
}

class B {
    public $s_b = "asdf";
    public $a_b;
    public $arr_a_b = false;
    public function __construct() { 
        $this->a_b  = new A(); 
        $this->arr_a_b = [new A(), new A()];
    }
}


$b = new B();
var_dump(instance_to_array($b));
