The Zen of Numstore
===================

This style guide draws heavily on TigerBeetle's 
[Tiger Style](https://github.com/tigerbeetle/tigerbeetle/blob/main/docs/TIGER_STYLE.md) and NASA's 
[Power of 10 rules for safety-critical code](https://en.wikipedia.org/wiki/The_Power_of_10:_Rules_for_Developing_Safety-Critical_Code).

1. No dynamic memory allocation after initialization        - In Progress
2. Simple control flow only — no `goto`                     - In Progress
3. Use modern tooling                                       - In Progress
4. Bound anything that can be bounded                       - In Progress
5. No function longer than 100 lines                        - In Progress
6. At most one level of pointer indirection                 - In Progress
7. Assertion density of 2%                                  - Unmeasured
8. Compile clean against all warnings                       - Fix: `-falignment`
9. No dependencies                                          - Complete

Developer Rules:

1. Keep the code in a better place than when you found it 
