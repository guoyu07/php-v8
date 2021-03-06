--TEST--
Check whether all method parameters have valid type
--SKIPIF--
<?php if (!extension_loaded("v8")) print "skip"; ?>
--FILE--
<?php
$re = new ReflectionExtension('v8');

$classes = $re->getClasses();


class Verifier
{
    private $invalid = [];

    public function verifyClass(ReflectionClass $class)
    {
        foreach ($class->getMethods() as $m) {
            $this->verifyMethod($m);
        }
    }

    public function verifyMethod(ReflectionMethod $method)
    {
        foreach ($method->getParameters() as $p) {
            $this->verifyParameter($p);
        }
    }

    public function verifyParameter(ReflectionParameter $parameter)
    {
        $type = $parameter->getType();

        if (!$type || $type->isBuiltin()) {
            return;
        }

        if (!(class_exists($type))) {
            $method_name = $parameter->getDeclaringClass()->getName() . '::' . $parameter->getDeclaringFunction()->getName();
            $param_name  = $parameter->getName();

            $shortcut = $method_name . '/' . $param_name;

            if (isset($this->invalid[$shortcut])) {
                return;
            }

            $this->invalid[$shortcut] = true;

            echo "{$method_name}() method's parameter {$parameter->getName()} has invalid type ($type)", PHP_EOL;
        }
    }

    public function isValid()
    {
        return empty($this->invalid);
    }
}

$v = new Verifier();


foreach ($classes as $c) {
    $v->verifyClass($c);
}

if ($v->isValid()) {
    echo 'All method parameters are valid', PHP_EOL;
}

?>
--EXPECT--
All method parameters are valid
