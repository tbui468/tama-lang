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
                            x: int = 224
                            y: int = 2 + 4
                            while x < a {
                                x = x + 1 
                            }
                            return x + b
                        }
                        main: () -> int {
                            z: int = 10-2
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
