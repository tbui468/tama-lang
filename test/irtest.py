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


tests = [
            ("return code", 0,
                [
                    ("main.tmd",
                        """
                        double: (a: int, b: int) -> int {
                            x: int = 4 + 38
                            while x < 10 {
                                x = x + 1
                            }
                            return x + b
                        }
                        main: () -> int {
                            y: int = 0
                            if y < 10 {
                                y = 1
                            } else {
                                y = 42
                            }
                            double(1, y)
                            return double(0, 21)
                        }
                        """
                    )
                ]
            ),
        ]


                        #a: int = 1 + 2 * 3
                        #b: int = 4
for data in tests:
    test(data)

print("Tests passed:", correct, "/", len(tests))