import subprocess

global correct
correct = 0

def test(data):
    with open("main.tmd", "w") as f:
        f.write(data[2])

    subprocess.run("./../build/src/tama main.tmd > /dev/null", shell=True)
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
             """main: () -> int {
                    return 0
                }"""
            ),
            ("sign integer arithmetic", 0, 
             """main: () -> int {
                    x: int = 10
                    y: int = 10
                    return -100 + x * y - x + y - y / x + x / y
                }"""
            ),
            ("conditionals", 0,
             """main: () -> int {
                    if true {
                        return 0
                    } else {
                        return 1
                    }
                }"""
            ),
            ("boolean operators", 0, 
             """main: () -> int {
                    if (true or false) and true {
                        return 0
                    }
                }"""
            ),
            ("while loops", 0, 
             """main: () -> int {
                    x: int = 9
                    while x > 0 {
                        x = x - 1
                    }
                    return x
                }"""
            ),
        ]

for data in tests:
    test(data)

print("Tests passed:", correct, "/", len(tests))
