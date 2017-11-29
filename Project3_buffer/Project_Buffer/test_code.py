#!/usr/bin/python3

'''
    test program
'''

from subprocess import Popen, PIPE
import os
from random import shuffle
from timeit import default_timer as timer
from math import log, floor
from time import sleep
import struct
import fcntl

INSERT_CMD_FMTS = "i %d %s\n"
DELETE_CMD_FMTS = "d %d\n"
FIND_CMD_FMTS = "f %d\n"
QUIT_CMD_FMTS = "q\n"
FIND_RESULT_FMTS = "key %d, value [%s]."
NOT_FOUND_RESULT = "Record not found under key %d."
RESULT_FMTS = "Result: %d/%d (%.2f) %.2f secs"

TARGET_DB_NAME = 'test.db'
BACKUP_DB_NAME = 'test.db.bak'
LAST_TEST_DB_NAME = 'last_test.db'
EXECUTABLE_NAME = 'main'

SMALL_CASE = 2 ** 5
MEDIUM_CASE = 2 ** 10
LARGE_CASE = 2 ** 15


def test_case(arr):
    os.system('rm -f ' + TARGET_DB_NAME)
    sleep(1)
    f = open("log.txt", "w")
    p = Popen(['-u', "./" + EXECUTABLE_NAME], stdin=PIPE, stdout=PIPE, shell=True)
    succ = 0;

    # insert start
    start = timer()

    for i in arr:
        input_d = INSERT_CMD_FMTS % (i, 'a' + str(i))
        f.write(input_d)
        p.stdin.write(input_d.encode("utf-8"))
        p.stdin.flush()

    '''p.stdin.write("c".encode("utf-8"))'''
    end = timer()

    for i in arr:
        input_d = FIND_CMD_FMTS % (i)
        f.write(input_d)
        p.stdin.write(input_d.encode("utf-8"))
        p.stdin.flush()
        result = p.stdout.readline().decode('utf-8').strip()
        '''result = nonBlockReadline(p.stdout)'''
        '''print(result)'''
        '''print((FIND_RESULT_FMTS % (i, 'a' + str(i))).strip())'''
        if (result == (FIND_RESULT_FMTS % (i, 'a' + str(i))).strip()):
            succ += 1
    '''f.write(QUIT_CMD_FMTS)'''
    p.stdin.write(QUIT_CMD_FMTS.encode('utf-8'))
    f.close()

    os.system("cp " + TARGET_DB_NAME + " " + LAST_TEST_DB_NAME)

    return succ, end - start


def test_case_seq(casename, case_size):
    print(casename + " Test")

    result, elapse = test_case(range(case_size))

    print(RESULT_FMTS % (result, case_size, float(result) / case_size * 100, elapse))
    if (result != case_size):
        print("FAILED - Stop Testing")
        exit()


def test_case_rnd(casename, case_size):
    print(casename + " Test")
    arr = list(range(case_size))
    shuffle(arr)

    result, elapse = test_case(arr)

    print(RESULT_FMTS % (result, case_size, float(result) / case_size * 100, elapse))
    if (result != case_size):
        print("FAILED - Stop Testing")
        exit()


def test_delete(remain_rec, delete_rec):
    os.system('rm -f ' + TARGET_DB_NAME)
    os.system('cp ' + BACKUP_DB_NAME + ' ' + TARGET_DB_NAME)
    sleep(0.1)
    f = open("log.txt", "w")
    p = Popen("./" + EXECUTABLE_NAME, stdin=PIPE, stdout=PIPE, shell=True)
    succ = 0;

    # insert start
    start = timer()

    for i in delete_rec:
        input_d = DELETE_CMD_FMTS % (i)
        f.write(input_d)
        p.stdin.write(input_d.encode("utf-8"))

        p.stdin.flush()

    end = timer()

    remain_rec.sort()
    delete_rec.sort()
    for i in remain_rec:
        input_d = FIND_CMD_FMTS % (i)
        f.write(input_d)
        p.stdin.write(input_d.encode("utf-8"))
        p.stdin.flush()
        result = p.stdout.readline().decode('utf-8').strip()

        if (result == (FIND_RESULT_FMTS % (i, 'a' + str(i))).strip()):
            succ += 1

    for i in delete_rec:
        input_d = FIND_CMD_FMTS % (i)
        f.write(input_d)
        p.stdin.write(input_d.encode("utf-8"))
        p.stdin.flush()
        result = p.stdout.readline().decode('utf-8').strip()

        if (result == (NOT_FOUND_RESULT % (i)).strip()):
            succ += 1

    p.stdin.write(QUIT_CMD_FMTS.encode('utf-8'))
    f.close()

    return succ, end - start


def test_delete_random(casename, pick):
    print(casename + " Test")
    arr = list(range(LARGE_CASE))
    shuffle(arr)

    result, elapse = test_delete(arr[pick:], arr[:pick])
    print(RESULT_FMTS % (result, LARGE_CASE, float(result) / (LARGE_CASE) * 100, elapse))
    if (result != LARGE_CASE):
        print("FAILED - Stop Testing")
        exit()


def test_delete_seq():
    print("Delete All Records Sequantial")
    arr = list(range(LARGE_CASE))

    result, elapse = test_delete([], arr)
    print(RESULT_FMTS % (result, LARGE_CASE, float(result) / (LARGE_CASE) * 100, elapse))
    if (result != LARGE_CASE):
        print("FAILED Delete Seq - Stop Testing")
        exit()


def test_delete_rev():
    print("Delete All Records Reversal")
    arr = list(range(LARGE_CASE))
    arr.reverse()

    result, elapse = test_delete([], arr)
    print(RESULT_FMTS % (result, LARGE_CASE, float(result) / (LARGE_CASE) * 100, elapse))
    if (result != LARGE_CASE):
        print("FAILED - Stop Testing")
        exit()


def test_delete_chunk():
    print("Delete Chunk Records randomly")
    arr = list(range(LARGE_CASE))
    exp = floor(log(LARGE_CASE, 2) / 2)
    start = (int)(2 ** exp)
    end = (int)(2 ** (exp + 1))

    deleted = arr[start: end]
    arr = arr[:start] + arr[end:]

    result, elapse = test_delete(arr, deleted)
    print(RESULT_FMTS % (result, LARGE_CASE, float(result) / (LARGE_CASE) * 100, elapse))
    if (result != LARGE_CASE):
        print("FAILED - Stop Testing")
        exit();


def make_backup():
    os.system('cp ' + LAST_TEST_DB_NAME + ' ' + BACKUP_DB_NAME)


'''os.system('make clean > /dev/null')
os.system('make test > /dev/null')
'''
try:
    os.remove(TARGET_DB_NAME)
except:
    pass

print("-------------- Sequantial Insert Test --------------")
test_case_seq("Small(2^10)", SMALL_CASE)
test_case_seq("Medium(2^15)", MEDIUM_CASE)
test_case_seq("Large(2^18)", LARGE_CASE)

print("--------------   Random Insert Test   --------------")
test_case_rnd("Small(2^10)", SMALL_CASE)
test_case_rnd("Medium(2^15)", MEDIUM_CASE)
test_case_rnd("Large(2^18)", LARGE_CASE)

sleep(0.1)

make_backup()

print("--------------      Delete Test       --------------")
test_delete_seq()
test_delete_rev()

test_delete_chunk()
test_delete_random("Random_Delete_Small(2^10)", SMALL_CASE)
test_delete_random("Random_Delete_Medium(2^15)", MEDIUM_CASE)
test_delete_random("Random_Delete_Large(2^17)", int(LARGE_CASE / 2))
test_delete_random("Random_Delete_ALL(2^18)", LARGE_CASE)
