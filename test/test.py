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
    cmd += " > /dev/null"

    subprocess.call(cmd, shell=True)
    subprocess.call("chmod +x out.exe", shell=True)
    p = subprocess.call("./out.exe", shell=True)

    name = "[" + data[0] + "]"
    result = "Failed"
    if p == data[1]:
        result = "Passed"
        global correct
        correct += 1

    print(name.ljust(40, " "), result)

    #cleanup
    for src in data[2]:
        subprocess.call("rm " + src[0], shell=True)
        subprocess.call("rm " + src[0][:-4] + ".asm", shell=True)
        subprocess.call("rm " + src[0][:-4] + ".obj", shell=True)

    subprocess.call("rm out.exe", shell=True)


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



failed_test = ("sign integer arithmetic", 0, 
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
            )

for data in tests:
    test(data)

print("Tests passed:", correct, "/", len(tests))
