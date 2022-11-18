import subprocess

global correct
correct = 0

def test(data):
    for src in data[2]:
        with open(src[0], "w") as f:
            f.write(src[1].strip())

    cmd = "./../build/src/tama"
    for src in data[2]:
        cmd += " " + src[0]
    #cmd += " > /dev/null"

    cp = subprocess.call(cmd, shell=True)
    subprocess.call("chmod +x out.exe", shell=True)
    p = subprocess.call("./out.exe", shell=True)

    name = "    [" + data[0] + "]"
    result = "Failed"
    if p == data[1] and cp == 0:
        result = "Passed"
        global correct
        correct += 1

    print(name.ljust(40, " "), result)

    #cleanup
    """
    for src in data[2]:
        subprocess.call("rm " + src[0], shell=True)
        subprocess.call("rm " + src[0][:-4] + ".asm", shell=True)
        subprocess.call("rm " + src[0][:-4] + ".obj", shell=True)

    subprocess.call("rm out.exe", shell=True)
    """

function_tests = [
            ("zero return value", 0,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            return 0
                        }
                        """
                    )
                ]
            ),
            ("positive return value", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            return 1
                        }
                        """
                    )
                ]
            ),
            ("true return value", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            return 1
                        }
                        """
                    )
                ]
            ),
            ("false return value", 0,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            return 0
                        }
                        """
                    )
                ]
            ),
            ("nil return value", 0,
                [
                    ("main.tmd",
                        """
                        main: () -> nil {
                            return nil
                        }
                        """
                    )
                ]
            ),
            ("declare function with parameters", 0,
                [
                    ("main.tmd",
                        """
                        myadd: (a: int, b: int) -> int {
                            return a + b
                        }
                        main: () -> int {
                            return 0
                        }
                        """
                    )
                ]
            ),
            ("call function with parameters", 0,
                [
                    ("main.tmd",
                        """
                        myadd: (a: int, b: int) -> int {
                            return a + b
                        }
                        main: () -> int {
                            return myadd(-5, 5)
                        }
                        """
                    )
                ]
            ),
        ]
arithmetic_expr_tests = [
            ("signed integer arithmetic", 0,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            return 10 + 16 / 2 - 3 * 6
                        }
                        """
                    )
                ]
            ),
        ]
boolean_expr_tests = [
            ("boolean false", 0,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return false
                        }
                        """
                    )
                ]
            ),
            ("boolean true", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return true
                        }
                        """
                    )
                ]
            ),
            ("< -> true", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return 5 < 10
                        }
                        """
                    )
                ]
            ),
            ("< -> false", 0,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return 10 < 5
                        }
                        """
                    )
                ]
            ),
            ("<= when left is less -> true", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return 5 <= 10
                        }
                        """
                    )
                ]
            ),
            ("<= when left is equal -> true", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return 5 <= 5
                        }
                        """
                    )
                ]
            ),
            ("<= when left is greater -> false", 0,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return 10 <= 5
                        }
                        """
                    )
                ]
            ),
            ("> -> true", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return 10 > 5
                        }
                        """
                    )
                ]
            ),
            ("> -> false", 0,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return 5 > 10
                        }
                        """
                    )
                ]
            ),
            (">= when left is greater -> true", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return 10 >= 5
                        }
                        """
                    )
                ]
            ),
            (">= when left is equal -> true", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return 5 >= 5
                        }
                        """
                    )
                ]
            ),
            (">= when left is less -> false", 0,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return 5 >= 10
                        }
                        """
                    )
                ]
            ),
            ("== -> true", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return 0 == 0
                        }
                        """
                    )
                ]
            ),
            ("== -> false", 0,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return 0 == 1
                        }
                        """
                    )
                ]
            ),
            ("!= -> true", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return 0 != 1
                        }
                        """
                    )
                ]
            ),
            ("!= -> false", 0,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return 0 != 0
                        }
                        """
                    )
                ]
            ),
            ("false and false", 0,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return false and false
                        }
                        """
                    )
                ]
            ),
            ("false and true", 0,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return false and true
                        }
                        """
                    )
                ]
            ),
            ("true and false", 0,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return true and false
                        }
                        """
                    )
                ]
            ),
            ("true and true", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return true and true
                        }
                        """
                    )
                ]
            ),
            ("false or false", 0,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return false or false
                        }
                        """
                    )
                ]
            ),
            ("false or true", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return false or true
                        }
                        """
                    )
                ]
            ),
            ("true or false", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return true or false
                        }
                        """
                    )
                ]
            ),
            ("true or true", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> bool {
                            return true or true
                        }
                        """
                    )
                ]
            ),
        ]


variable_tests = [
            ("declaration", 0,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            x: int = 1 + 4
                            return 0
                        }
                        """
                    )
                ]
            ),
            ("access", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            x: int = 1
                            return x
                        }
                        """
                    )
                ]
            ),
            ("assignment", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            x: int = 0
                            x = 1
                            return x
                        }
                        """
                    )
                ]
            ),
            ("access from inner scope", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            x: int = 1
                            {
                                return x
                            }
                        }
                        """
                    )
                ]
            ),
            ("assignment from inner scope", 2,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            x: int = 1
                            {
                                x = 2
                            }
                            return x
                        }
                        """
                    )
                ]
            ),
            ("shadow outer scope", 2,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            x: int = 1
                            {
                                x: int = 2
                                return x
                            }
                        }
                        """
                    )
                ]
            ),
            ("shadowed by inner scope", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            x: int = 1
                            {
                                x: int = 2
                                x = 3
                            }
                            return x
                        }
                        """
                    )
                ]
            ),
        ]


module_tests = [
            ("import module", 0,
                [
                    ("main.tmd",
                        """
                        import math
                        main: () -> int {
                            return 0
                        }
                        """
                    ),
                    ("math.tmd",
                        """
                        myadd: (a: int, b:int) -> int {
                            return a + b
                        }
                        """
                    )
                ]
            ),
            ("call imported module function", 0,
                [
                    ("main.tmd",
                        """
                        import math
                        main: () -> int {
                            return myadd(-5, 5)
                        }
                        """
                    ),
                    ("math.tmd",
                        """
                        myadd: (a: int, b:int) -> int {
                            return a + b
                        }
                        """
                    )
                ]
            ),
        ]

conditional_tests = [
            ("if with true condition", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            if 0 < 10 {
                                return 1
                            }
                            return 2
                        }
                        """
                    )
                ]
            ),
            ("if with false condition", 2,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            if 0 > 10 {
                                return 1
                            }
                            return 2
                        }
                        """
                    )
                ]
            ),
            ("if/else true condition", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            if 0 < 10 {
                                return 1
                            } else {
                                return 2
                            }
                        }
                        """
                    )
                ]
            ),
            ("if/else false condition", 2,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            if 0 > 10 {
                                return 1
                            } else {
                                return 2
                            }
                        }
                        """
                    )
                ]
            ),
            ("if/else skip else", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            x: int = 0
                            if 0 < 10 {
                                x = x + 1
                            } else {
                                x = x + 2
                            }
                            return x
                        }
                        """
                    )
                ]
            ),
        ]


while_tests = [
            ("run zero times", 0,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            while 0 > 10 {
                                return 1
                            }
                            return 0
                        }
                        """
                    )
                ]
            ),
            ("run once", 1,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            x: int = 0
                            while x < 1 {
                                x = x + 1
                            }
                            return x
                        }
                        """
                    )
                ]
            ),
            ("run more than once", 5,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            x: int = 0
                            while x < 5 {
                                x = x + 1
                            }
                            return x
                        }
                        """
                    )
                ]
            ),
        ]

"""
print("--Functions--")
for data in function_tests:
    test(data)

print("--Arithmetic Expressions--")
for data in arithmetic_expr_tests:
    test(data)

print("--Logical Expressions--")
for data in boolean_expr_tests:
    test(data)

print("--Variables--")
for data in variable_tests:
    test(data)

print("--Modules--")
for data in module_tests:
    test(data)

print("--Conditionals--")
for data in conditional_tests:
    test(data)

print("--While Loops--")
for data in while_tests:
    test(data)
"""
print("--(test...for testing)--")
test(        ("test set", 6,
            [
                ("main.tmd",
                    """
                    mysub: (a: int, b: int) -> int {
                        if a < b {
                            x: int = 3
                        } else {
                            x: int = 10
                        }
                        return a - b
                    }
                    myadd: (a: int, b: int) -> int {
                        return a + b
                    }
                    main: () -> int {
                        x: int = 0
                        while x < 10 {
                            if x < 0 {
                                x = myadd(x, 1)
                            } else {
                                x = myadd(x, 2)
                            }
                            if x > 0 {
                                return 0
                            }
                        }
                        return x
                        z: int = 10
                        return z
                    }
                    """
                )
            ]
        ),)

print("Tests passed:", correct, "/", len(function_tests) + len(arithmetic_expr_tests) + len(boolean_expr_tests) + len(variable_tests) + len(module_tests) + len(conditional_tests) + len(while_tests))
