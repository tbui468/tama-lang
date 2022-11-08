import subprocess

global correct
correct = 0

def test(data):
    files = []
    for src in data[2]:
        with open(src[0], "w") as f:
            f.write(src[1].strip())

    cmd = "./../build/src/tama"
    for src in data[2]:
        cmd += " " + src[0]
    cmd += " > /dev/null"
    subprocess.run(cmd, shell=True)
    p = subprocess.run("./out.exe", shell=True)

    name = "[" + data[0] + "]"
    result = "Failed"
    if p.returncode == data[1]:
        result = "Passed"
        global correct
        correct += 1
    print(name.ljust(40, " "), result)


tests = [
            ("main function return code", 0,
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
            ("sign integer arithmetic", 0, 
                [
                    ("main.tmd",
                         """
                         main: () -> int {
                            x: int = 10
                            y: int = 10
                            return -100 + x * y - x + y - y / x + x / y
                         }
                         """
                    )
                ]
            ),
            ("conditionals", 0,
                [
                    ("main.tmd",
                         """
                         main: () -> int {
                            if true {
                                return 0
                            } else {
                                return 1
                            }
                         }
                         """
                    )
                ]
            ),
            ("boolean operators", 0, 
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            if (true or false) and true {
                                return 0
                            }
                        }
                        """
                    )
                ]
            ),
            ("while loops", 0, 
                [
                    ("main.tmd",
                         """
                         main: () -> int {
                                x: int = 9
                                while x > 0 {
                                    x = x - 1
                                }
                                return x
                         }
                         """
                    )
                ]
            ),
            ("functions", 0, 
                [
                    ("main.tmd",
                         """
                         add: (a: int, b: int) -> int {
                                return a + b
                            }

                            main: () -> int {
                                return add(-10, 10)
                            }
                          """
                    )
                ]
            ),
            ("module import", 0,
                [
                    ("main.tmd", 
                        """
                        import math
                        main: () -> int {
                            return add(-10, 10)
                        }
                        """
                    ),
                    ("math.tmd",
                        """
                        add: (a: int, b: int) -> int {
                        return a + b
                        }
                        """
                    ),
                ]
            ),
        ]


for data in tests:
    test(data)

print("Tests passed:", correct, "/", len(tests))
