#!/usr/bin/env python
# encoding: utf-8

class Countdown(object):
    def __init__(self, val):
        self._val = val

    def sub(self):
        tmp = self._val
        self._val -= 1
        return tmp

    def add(self):
        tmp = self._val
        self._val += 1
        return tmp

    # iter 函数一个鲜为人知的特性是它接受一个可选的 callable 对象
    # 和一个标记(结尾)值作为输入参数。
    # 当以这种方式使用的时候，它会创建一个迭代器， 这个迭代器会不断调用 callable 对象直到返回值和标记值相等为止。
    def __iter__(self):
        return iter(self.sub, 0)
        #while i >= 0:
        #    yield i
        #    i -= 1

    # 注意是__reversed__, not __reverse__
    def __reversed__(self):
        return iter(self.add, 5)

C = Countdown(5)
for k in C:
    print k,
print

C = Countdown(0)
for k in reversed(C):
    print k,
