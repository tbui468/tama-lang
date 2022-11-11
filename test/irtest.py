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
                        main: () -> int {
                            x: int = 1
                            {
                                x = 2
                                print(x)
                                x: int = 3
                                x = 4
                            }
                            x = 5
                            return 0
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
