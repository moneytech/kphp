@kphp_should_fail
/Expected type:.+void/
KPHP_REQUIRE_FUNCTIONS_TYPING=1
<?php

// @return void unless specified, even if others are set

function getInt() : int { return 4; }

/** @param int|false $b */
function f4(int $a, $b) {
  return getInt();
}

f4(3, false);
