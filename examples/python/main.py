import os

assert os.environ.get("BASIC_ENV") == "true"
assert os.environ.get("QUOTES") == "sad\"wow\"bak"
assert os.environ.get(
    "MULTI_LINE_KEY") == "ssh-rsa BBBBPl1P1AD+jk/3Lf3Dw== test@example.com"
assert os.environ.get(
    "MONGOLAB_URI") == ("mongodb://root:password@abcd1234.mongolab.com:"
                        "12345/localhost")

print("Successfully assigned ENVs to process!")
