Import("env")
import datetime

now = datetime.datetime.now()
env.Append(CPPDEFINES=[
    ("BUILD_YEAR", now.year),
    ("BUILD_MONTH", now.month),
    ("BUILD_DAY", now.day),
    ("BUILD_HOUR", now.hour),
    ("BUILD_MINUTE", now.minute),
    ("BUILD_SECOND", now.second),
])
