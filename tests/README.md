# Visible Checks

Run the local visible check with:

```bash
./scripts/test.sh
```

The script builds the project, runs `./bin/msgq_lab`, and performs an **exact**
string comparison against the expected output:

```
child received: sensor id=7 value=42
parent: sent reading
child received hi-pri: value=99
child received lo-pri: value=10
queue full: EAGAIN detected
oversized send: EMSGSIZE detected
all tasks done
```

The test exits with status 0 only when the program output matches exactly.
Any extra or missing line, or a line with different content, will fail the check.
