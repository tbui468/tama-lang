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

    cp = subprocess.call(cmd, shell=True)
    subprocess.call("chmod +x out.exe", shell=True)
    p = subprocess.call("./out.exe", shell=True)

    name = "[" + data[0] + "]"
    result = "Failed"
    if p == data[1] and cp == 0:
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
            ("return code", 0,
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
            ("arithmetic", 0,
                [
                    ("main.tmd",
                        """
                        main: () -> int {
                            return 1 + 2 / 1 - 3 * 1
                        }
                        """
                    )
                ]
            ),
        ]


for data in tests:
    test(data)

print("Tests passed:", correct, "/", len(tests))
