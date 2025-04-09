#!/usr/bin/env python3

def f(n):
        a, b = 1, 2
        for i in range(0, n):
                # a, b = b, a + b
                _temp = a
                a = b
                b = _temp + a
        return a

for n in range(11):
    print(f(n))
