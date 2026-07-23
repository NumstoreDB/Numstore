/// Copyright 2026 Theo Lincke
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
use std::ops::{Add, Rem, RemAssign, Sub};

/// A key is anything that has basic arithmetic
/// to be hashed into a bin. Must be copyable
/// in order to move it between bins and must be
/// ordered
pub trait Key:
    Copy + Add<Output = Self> + Sub<Output = Self> + Rem<Output = Self> + RemAssign + PartialOrd
{
    /// Initial value
    const ZERO: Self;

    /// In order to add one to value
    const ONE: Self;

    /// Convert self to an index
    fn as_index(self) -> usize;
}

/// A value just needs to be able to be copied
pub trait Value: Copy {}

// =============================================================================
// SECTION: Some implementation of common keys
// =============================================================================

impl Key for u8 {
    const ZERO: Self = 0;
    const ONE: Self = 1;
    fn as_index(self) -> usize {
        self as usize
    }
}
impl Key for u16 {
    const ZERO: Self = 0;
    const ONE: Self = 1;
    fn as_index(self) -> usize {
        self as usize
    }
}
impl Key for u32 {
    const ZERO: Self = 0;
    const ONE: Self = 1;
    fn as_index(self) -> usize {
        self as usize
    }
}
impl Key for u64 {
    const ZERO: Self = 0;
    const ONE: Self = 1;
    fn as_index(self) -> usize {
        self as usize
    }
}
impl Key for usize {
    const ZERO: Self = 0;
    const ONE: Self = 1;
    fn as_index(self) -> usize {
        self
    }
}

impl Value for u8 {}
impl Value for u16 {}
impl Value for u32 {}
impl Value for u64 {}
impl Value for usize {}

impl Value for i8 {}
impl Value for i16 {}
impl Value for i32 {}
impl Value for i64 {}
impl Value for isize {}

// =============================================================================
// SECTION: Robin Hood Hash table start
// =============================================================================

/// Data is a tuple of key and value
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
struct RHData<K: Key, V: Value> {
    key: K,
    value: V,
}

/// A single frame has data and distance
/// from initial bucket and if it's present or not
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
struct RHFrame<K: Key, V: Value> {
    dib: usize,
    data: RHData<K, V>,
    present: bool,
}

/// The main struct
pub struct RobinHoodHt<K: Key, V: Value, const N: usize> {
    entries: [RHFrame<K, V>; N],
    len: usize,
}

impl<K: Key, V: Value, const N: usize> RobinHoodHt<K, V, N> {
    pub fn new(v: V) -> Self {
        RobinHoodHt {
            entries: [RHFrame {
                dib: 0,
                data: RHData {
                    key: K::ZERO,
                    value: v,
                },
                present: false,
            }; N],
            len: 0,
        }
    }
}

/// The result of an insert operation
#[derive(Debug, PartialEq)]
pub enum InsertRes {
    Success,
    Exists,
    Full,
}

/// The result of accessing (get or delete)
#[derive(Debug)]
pub enum AccessRes<V: Value> {
    Success(V),
    DoesntExist,
}

impl<K: Key, V: Value, const N: usize> RobinHoodHt<K, V, N> {
    /**
     * Key = 1
     * Value = 2
     *
     * [ _, _, _, _, _, _, _, _, _, _, _ ]
     *      ^
     *   initial bucket
     */
    pub fn insert(&mut self, key: K, value: V) -> InsertRes {
        assert!(self.len <= N);

        if self.len == N {
            return InsertRes::Full;
        }

        let mut dibn: usize = 0;
        let mut data = RHData { key, value };

        for _ in 0..N {
            let i = (data.key.as_index() + dibn) % N;

            /*
             * [ _, _, _, _, _, _, _, _, _, _, _ ]
             *      ^
             *   initial bucket is not present - just insert it
             * [ _, 1: 2, _, _, _, _, _, _, _, _, _ ]
             */
            if !self.entries[i].present {
                self.entries[i].data = data;
                self.entries[i].dib = dibn;
                self.entries[i].present = true;
                self.len += 1;
                return InsertRes::Success;
            }

            /*
             * Example 1:
             *
             * [ x, 0: 3, _, _, _, _, _, _, _, _, _ ]
             *      ^
             *      1: 2
             *   initial bucket has a 0 key (so it doesn't belong in index 1)
             *   dib(0: 3, 1) == 1, therefore, 0 is further from it's initial bucket than 1
             *
             *   dib(0: 3, 1) > dib(1: 2, 1) => 1 > 0
             *
             * Example 2:
             *
             * [ _, x, x, 2: 1, _, _, _, _, _, _, _ ]
             *            ^
             *            1: 2
             *   initial bucket has a 2 key (so it doesn't belong in index 3)
             *   dib(2: 1, 3) == 1,
             *
             *   dib(2: 1, 3) < dib(1: 2, 3) => 1 < 2
             *
             *   So we swap and continue moving with 2: 1
             *
             * [ _, x, x, 1: 2, _, _, _, _, _, _, _ ]
             *            ^
             *            2: 1
             */
            if self.entries[i].dib < dibn {
                let temp_data: RHData<K, V> = self.entries[i].data;
                let temp_dib = self.entries[i].dib;

                self.entries[i].data = data;
                self.entries[i].dib = dibn;

                dibn = temp_dib;
                data = temp_data;
            }

            // Compare keys for duplicates
            if self.entries[i].data.key == data.key {
                return InsertRes::Exists;
            }

            dibn += 1;
        }

        unreachable!("Bounds check happened at the start");
    }

    pub fn get(&self, key: K) -> AccessRes<V> {
        if self.len == 0 {
            return AccessRes::DoesntExist;
        }

        let mut dibn: usize = 0;

        for i in 0..N {
            let i: usize = (key.as_index() + i) % N;

            // Not present — can't be here
            if !self.entries[i].present {
                return AccessRes::DoesntExist;
            }

            // DIB invariant broken — not here either
            if self.entries[i].dib < dibn {
                return AccessRes::DoesntExist;
            }

            if self.entries[i].data.key == key {
                return AccessRes::Success(self.entries[i].data.value);
            }

            dibn += 1;
        }

        AccessRes::DoesntExist
    }

    pub fn delete(&mut self, key: K) -> AccessRes<V> {
        if self.len == 0 {
            return AccessRes::DoesntExist;
        }

        let mut dibn: usize = 0;
        let mut i: usize = 0;

        while i < N {
            let _i: usize = (key.as_index() + i) % N;

            if !self.entries[_i].present {
                return AccessRes::DoesntExist;
            }

            if self.entries[_i].dib < dibn {
                return AccessRes::DoesntExist;
            }

            if self.entries[_i].data.key == key {
                let deleted_value = self.entries[_i].data.value;

                while i < N {
                    let hole: usize = (key.as_index() + i) % N;
                    let next: usize = (key.as_index() + i + 1) % N;

                    if !self.entries[next].present || self.entries[next].dib == 0 {
                        self.entries[hole].present = false;
                        assert!(self.len > 0);
                        self.len -= 1;
                        return AccessRes::Success(deleted_value);
                    }

                    self.entries[hole].data = self.entries[next].data;
                    assert!(self.entries[next].dib > 0);
                    self.entries[hole].dib = self.entries[next].dib - 1;

                    i += 1;
                }

                unreachable!();
            }

            dibn += 1;
            i += 1;
        }

        AccessRes::DoesntExist
    }

    pub fn insert_expect(&mut self, key: K, value: V) {
        let res = self.insert(key, value);
        assert!(matches!(res, InsertRes::Success));
    }

    pub fn get_expect(&self, key: K) -> V {
        let res = self.get(key);
        assert!(matches!(res, AccessRes::Success(_)));
        if let AccessRes::Success(x) = res {
            x
        } else {
            unreachable!()
        }
    }

    pub fn delete_expect(&mut self, key: K) -> V {
        let res = self.delete(key);
        assert!(matches!(res, AccessRes::Success(_)));
        if let AccessRes::Success(x) = res {
            x
        } else {
            unreachable!()
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn make_ht<const N: usize>() -> RobinHoodHt<usize, i32, N> {
        RobinHoodHt {
            entries: [RHFrame {
                dib: 0,
                data: RHData { key: 0, value: 0 },
                present: false,
            }; N],
            len: 0,
        }
    }

    #[test]
    fn test_insert_and_get() {
        let mut ht = make_ht::<16>();

        ht.insert_expect(3, 42);
        ht.insert_expect(7, 99);

        assert_eq!(ht.get_expect(3), 42);
        assert_eq!(ht.get_expect(7), 99);
        assert!(matches!(ht.get(99), AccessRes::DoesntExist));
    }

    #[test]
    fn test_duplicate_insert() {
        let mut ht = make_ht::<8>();

        assert_eq!(ht.insert(5, 10), InsertRes::Success);
        assert_eq!(ht.insert(5, 20), InsertRes::Exists);
        assert_eq!(ht.get_expect(5), 10);
    }

    #[test]
    fn test_delete() {
        let mut ht = make_ht::<16>();

        ht.insert_expect(1, 100);
        ht.insert_expect(2, 200);
        ht.insert_expect(3, 300);

        assert!(matches!(ht.delete(2), AccessRes::Success(200)));
        assert!(matches!(ht.get(2), AccessRes::DoesntExist));

        assert_eq!(ht.get_expect(1), 100);
        assert_eq!(ht.get_expect(3), 300);
    }

    #[test]
    fn test_delete_returns_correct_value_after_shift() {
        let mut ht = make_ht::<8>();

        // Order doesn't matter on hash collisions
        // Buckets aren't "Sticky" during delete

        ht.insert_expect(1, 1000);
        ht.insert_expect(25, 4000);
        ht.insert_expect(9, 2000);
        ht.insert_expect(17, 3000);

        assert_eq!(ht.get_expect(1), 1000);
        assert_eq!(ht.get_expect(25), 4000);
        assert_eq!(ht.get_expect(9), 2000);
        assert_eq!(ht.get_expect(17), 3000);

        // [ _, 1, 25, 17, 9, _, _, _ ]

        ht.insert_expect(3, 5000);

        {
            /*
             *      0  1   2   3
             * [ _, 1, 25, 9, 17,_ , _, _ ]
             *             ^
             *             3
             *      dib(9, 3) == 2
             *      dib(3, 3) == 0
             *      dib(9, 3) > dib(3, 3)
             *      DONT SWAP
             *
             *      0  1   2   3
             * [ _, 1, 25, 9, 17, _ , _, _ ]
             *                 ^
             *                 3
             *      dib(17, 4) == 3
             *      dib(3, 4) == 1
             *      dib(17, 4) > dib(3, 4)
             *      DONT SWAP
             *
             * [ _, 1, 25, 9, 17, 3, _, _ ]
             */

            assert_eq!(ht.entries[0].present, false);
            assert_eq!(
                ht.entries[1],
                RHFrame {
                    dib: 0,
                    data: RHData {
                        key: 1,
                        value: 1000
                    },
                    present: true
                }
            );
            assert_eq!(
                ht.entries[2],
                RHFrame {
                    dib: 1,
                    data: RHData {
                        key: 25,
                        value: 4000
                    },
                    present: true
                }
            );
            assert_eq!(
                ht.entries[3],
                RHFrame {
                    dib: 2,
                    data: RHData {
                        key: 9,
                        value: 2000
                    },
                    present: true
                }
            );
            assert_eq!(
                ht.entries[4],
                RHFrame {
                    dib: 3,
                    data: RHData {
                        key: 17,
                        value: 3000
                    },
                    present: true
                }
            );
            assert_eq!(
                ht.entries[5],
                RHFrame {
                    dib: 2,
                    data: RHData {
                        key: 3,
                        value: 5000
                    },
                    present: true
                }
            );
            assert_eq!(ht.entries[6].present, false);
            assert_eq!(ht.entries[7].present, false);

            assert_eq!(ht.get_expect(1), 1000);
            assert_eq!(ht.get_expect(25), 4000);
            assert_eq!(ht.get_expect(9), 2000);
            assert_eq!(ht.get_expect(17), 3000);
            assert_eq!(ht.get_expect(3), 5000);
        }

        assert!(matches!(ht.delete(25), AccessRes::Success(4000)));

        {
            // [ _, 1, 9, 17, 3, _, _, _, _, _ ]

            assert_eq!(ht.entries[0].present, false);
            assert_eq!(
                ht.entries[1],
                RHFrame {
                    dib: 0,
                    data: RHData {
                        key: 1,
                        value: 1000
                    },
                    present: true
                }
            );
            assert_eq!(
                ht.entries[2],
                RHFrame {
                    dib: 1,
                    data: RHData {
                        key: 9,
                        value: 2000
                    },
                    present: true
                }
            );
            assert_eq!(
                ht.entries[3],
                RHFrame {
                    dib: 2,
                    data: RHData {
                        key: 17,
                        value: 3000
                    },
                    present: true
                }
            );
            assert_eq!(
                ht.entries[4],
                RHFrame {
                    dib: 1,
                    data: RHData {
                        key: 3,
                        value: 5000
                    },
                    present: true
                }
            );
            assert_eq!(ht.entries[5].present, false);
            assert_eq!(ht.entries[6].present, false);
            assert_eq!(ht.entries[7].present, false);

            assert_eq!(ht.get_expect(1), 1000);
            assert!(matches!(ht.get(25), AccessRes::DoesntExist));
            assert_eq!(ht.get_expect(9), 2000);
            assert_eq!(ht.get_expect(17), 3000);
            assert_eq!(ht.get_expect(3), 5000);
        }

        assert!(matches!(ht.delete(9), AccessRes::Success(2000)));

        {
            // [ _, 1, 17, 3, _, _, _, _ ]

            assert_eq!(ht.entries[0].present, false);
            assert_eq!(
                ht.entries[1],
                RHFrame {
                    dib: 0,
                    data: RHData {
                        key: 1,
                        value: 1000
                    },
                    present: true
                }
            );
            assert_eq!(
                ht.entries[2],
                RHFrame {
                    dib: 1,
                    data: RHData {
                        key: 17,
                        value: 3000
                    },
                    present: true
                }
            );
            assert_eq!(
                ht.entries[3],
                RHFrame {
                    dib: 0,
                    data: RHData {
                        key: 3,
                        value: 5000
                    },
                    present: true
                }
            );
            assert_eq!(ht.entries[4].present, false);
            assert_eq!(ht.entries[5].present, false);
            assert_eq!(ht.entries[6].present, false);
            assert_eq!(ht.entries[7].present, false);

            assert_eq!(ht.get_expect(1), 1000);
            assert!(matches!(ht.get(25), AccessRes::DoesntExist));
            assert!(matches!(ht.get(9), AccessRes::DoesntExist));
            assert_eq!(ht.get_expect(17), 3000);
            assert_eq!(ht.get_expect(3), 5000);
        }

        assert!(matches!(ht.delete(1), AccessRes::Success(1000)));

        {
            // [ _, 17, _, 3, _, _, _, _ ]

            assert_eq!(ht.entries[0].present, false);
            assert_eq!(
                ht.entries[1],
                RHFrame {
                    dib: 0,
                    data: RHData {
                        key: 17,
                        value: 3000
                    },
                    present: true
                }
            );
            assert_eq!(ht.entries[2].present, false);
            assert_eq!(
                ht.entries[3],
                RHFrame {
                    dib: 0,
                    data: RHData {
                        key: 3,
                        value: 5000
                    },
                    present: true
                }
            );
            assert_eq!(ht.entries[4].present, false);
            assert_eq!(ht.entries[5].present, false);
            assert_eq!(ht.entries[6].present, false);
            assert_eq!(ht.entries[7].present, false);

            assert!(matches!(ht.get(1), AccessRes::DoesntExist));
            assert!(matches!(ht.get(25), AccessRes::DoesntExist));
            assert!(matches!(ht.get(9), AccessRes::DoesntExist));
            assert_eq!(ht.get_expect(17), 3000);
            assert_eq!(ht.get_expect(3), 5000);
        }

        assert!(matches!(ht.delete(17), AccessRes::Success(3000)));

        {
            // [ _, _, _, 3, _, _, _, _ ]

            assert_eq!(ht.entries[0].present, false);
            assert_eq!(ht.entries[1].present, false);
            assert_eq!(ht.entries[2].present, false);
            assert_eq!(
                ht.entries[3],
                RHFrame {
                    dib: 0,
                    data: RHData {
                        key: 3,
                        value: 5000
                    },
                    present: true
                }
            );
            assert_eq!(ht.entries[4].present, false);
            assert_eq!(ht.entries[5].present, false);
            assert_eq!(ht.entries[6].present, false);
            assert_eq!(ht.entries[7].present, false);

            assert!(matches!(ht.get(1), AccessRes::DoesntExist));
            assert!(matches!(ht.get(25), AccessRes::DoesntExist));
            assert!(matches!(ht.get(9), AccessRes::DoesntExist));
            assert!(matches!(ht.get(17), AccessRes::DoesntExist));
            assert_eq!(ht.get_expect(3), 5000);
        }

        assert!(matches!(ht.delete(3), AccessRes::Success(5000)));

        {
            // [ _, _, _, _, _, _, _, _ ]

            assert_eq!(ht.entries[0].present, false);
            assert_eq!(ht.entries[1].present, false);
            assert_eq!(ht.entries[2].present, false);
            assert_eq!(ht.entries[3].present, false);
            assert_eq!(ht.entries[4].present, false);
            assert_eq!(ht.entries[5].present, false);
            assert_eq!(ht.entries[6].present, false);
            assert_eq!(ht.entries[7].present, false);

            assert!(matches!(ht.get(1), AccessRes::DoesntExist));
            assert!(matches!(ht.get(25), AccessRes::DoesntExist));
            assert!(matches!(ht.get(9), AccessRes::DoesntExist));
            assert!(matches!(ht.get(17), AccessRes::DoesntExist));
            assert!(matches!(ht.get(3), AccessRes::DoesntExist));
        }
    }

    #[test]
    fn test_full() {
        let mut ht = make_ht::<8>();

        ht.insert_expect(0, 1);
        ht.insert_expect(1, 1);
        ht.insert_expect(2, 1);
        ht.insert_expect(3, 1);
        ht.insert_expect(4, 1);
        ht.insert_expect(5, 1);
        ht.insert_expect(6, 1);
        ht.insert_expect(7, 1);

        assert_eq!(ht.insert(8, 1), InsertRes::Full);

        // [ 0, 1, 2, 3, 4, 5, 6, 7, 8 ]

        ht.delete_expect(0);
        ht.delete_expect(1);
        ht.delete_expect(2);
        ht.delete_expect(3);
        ht.delete_expect(4);
        ht.delete_expect(5);
        ht.delete_expect(6);
        ht.delete_expect(7);
    }

    #[test]
    fn test_absences() {
        let mut ht = make_ht::<8>();

        assert!(matches!(ht.get(3), AccessRes::DoesntExist)); // Not present
        assert!(matches!(ht.delete(3), AccessRes::DoesntExist)); // Not present

        ht.insert_expect(0, 1);
        ht.insert_expect(1, 1);
        ht.insert_expect(2, 1);

        assert!(matches!(ht.get(3), AccessRes::DoesntExist)); // Not present
        assert!(matches!(ht.get(9), AccessRes::DoesntExist)); // Present - still return DoesntExist
        assert!(matches!(ht.delete(3), AccessRes::DoesntExist)); // Not present
        assert!(matches!(ht.delete(9), AccessRes::DoesntExist)); // Present - still return DoesntExist
    }

    #[test]
    fn test_full_or_empty() {
        let mut ht = make_ht::<3>();

        // Insert into the same spots - and fill it up
        ht.insert_expect(0, 1);
        ht.insert_expect(3, 1);
        ht.insert_expect(6, 1);

        assert!(matches!(ht.insert(9, 1), InsertRes::Full));

        assert!(matches!(ht.get(12), AccessRes::DoesntExist)); // Full
        assert!(matches!(ht.delete(12), AccessRes::DoesntExist)); // Full
    }

    #[test]
    fn test_swap_up() {
        let mut ht = make_ht::<5>();
        ht.insert_expect(0, 1);
        ht.insert_expect(5, 1);
        ht.insert_expect(1, 1);
        ht.insert_expect(2, 1);

        ht.insert_expect(11, 1);

        {
            /*
             * [ 0, 5, 1, 2, _ ]
             *      ^
             *      11
             *    dib(5) = 1
             *    dib(11) = 0
             *    DONT SWAP
             *
             * [ 0, 5, 1, 2, _ ]
             *         ^
             *         11
             *    dib(11) = 1
             *    dib(1) = 1
             *    DONT SWAP
             *
             * [ 0, 5, 1, 2, _ ]
             *            ^
             *            11
             *    dib(11) = 2
             *    dib(2) = 1
             *    SWAP
             *
             * [ 0, 5, 1, 11, 2 ]
             */

            assert_eq!(
                ht.entries[0],
                RHFrame {
                    dib: 0,
                    data: RHData { key: 0, value: 1 },
                    present: true
                }
            );
            assert_eq!(
                ht.entries[1],
                RHFrame {
                    dib: 1,
                    data: RHData { key: 5, value: 1 },
                    present: true
                }
            );
            assert_eq!(
                ht.entries[2],
                RHFrame {
                    dib: 1,
                    data: RHData { key: 1, value: 1 },
                    present: true
                }
            );
            assert_eq!(
                ht.entries[3],
                RHFrame {
                    dib: 2,
                    data: RHData { key: 11, value: 1 },
                    present: true
                }
            );
            assert_eq!(
                ht.entries[4],
                RHFrame {
                    dib: 2,
                    data: RHData { key: 2, value: 1 },
                    present: true
                }
            );

            assert_eq!(ht.get_expect(0), 1);
            assert_eq!(ht.get_expect(5), 1);
            assert_eq!(ht.get_expect(1), 1);
            assert_eq!(ht.get_expect(11), 1);
            assert_eq!(ht.get_expect(2), 1);
        }

        assert!(matches!(ht.delete(0), AccessRes::Success(1)));

        {
            // [ 5, 1, 11, 2, _ ]
            assert_eq!(
                ht.entries[0],
                RHFrame {
                    dib: 0,
                    data: RHData { key: 5, value: 1 },
                    present: true
                }
            );
            assert_eq!(
                ht.entries[1],
                RHFrame {
                    dib: 0,
                    data: RHData { key: 1, value: 1 },
                    present: true
                }
            );
            assert_eq!(
                ht.entries[2],
                RHFrame {
                    dib: 1,
                    data: RHData { key: 11, value: 1 },
                    present: true
                }
            );
            assert_eq!(
                ht.entries[3],
                RHFrame {
                    dib: 1,
                    data: RHData { key: 2, value: 1 },
                    present: true
                }
            );
            assert_eq!(ht.entries[4].present, false);

            assert_eq!(ht.get_expect(5), 1);
            assert_eq!(ht.get_expect(1), 1);
            assert_eq!(ht.get_expect(11), 1);
            assert_eq!(ht.get_expect(2), 1);
            assert!(matches!(ht.get(0), AccessRes::DoesntExist));
        }

        assert!(matches!(ht.delete(5), AccessRes::Success(1)));

        {
            // [ _, 1, 11, 2, _ ]
            assert_eq!(ht.entries[0].present, false);
            assert_eq!(
                ht.entries[1],
                RHFrame {
                    dib: 0,
                    data: RHData { key: 1, value: 1 },
                    present: true
                }
            );
            assert_eq!(
                ht.entries[2],
                RHFrame {
                    dib: 1,
                    data: RHData { key: 11, value: 1 },
                    present: true
                }
            );
            assert_eq!(
                ht.entries[3],
                RHFrame {
                    dib: 1,
                    data: RHData { key: 2, value: 1 },
                    present: true
                }
            );
            assert_eq!(ht.entries[4].present, false);

            assert!(matches!(ht.get(0), AccessRes::DoesntExist));
            assert!(matches!(ht.get(5), AccessRes::DoesntExist));
            assert_eq!(ht.get_expect(1), 1);
            assert_eq!(ht.get_expect(11), 1);
            assert_eq!(ht.get_expect(2), 1);
        }

        assert!(matches!(ht.delete(1), AccessRes::Success(1)));

        {
            // [ _, 11, 2, _, _ ]
            assert_eq!(ht.entries[0].present, false);
            assert_eq!(
                ht.entries[1],
                RHFrame {
                    dib: 0,
                    data: RHData { key: 11, value: 1 },
                    present: true
                }
            );
            assert_eq!(
                ht.entries[2],
                RHFrame {
                    dib: 0,
                    data: RHData { key: 2, value: 1 },
                    present: true
                }
            );
            assert_eq!(ht.entries[3].present, false);
            assert_eq!(ht.entries[4].present, false);

            assert!(matches!(ht.get(0), AccessRes::DoesntExist));
            assert!(matches!(ht.get(5), AccessRes::DoesntExist));
            assert!(matches!(ht.get(1), AccessRes::DoesntExist));
            assert_eq!(ht.get_expect(11), 1);
            assert_eq!(ht.get_expect(2), 1);
        }

        assert!(matches!(ht.delete(11), AccessRes::Success(1)));

        {
            // [ _, _, 2, _, _ ]
            assert_eq!(ht.entries[0].present, false);
            assert_eq!(ht.entries[1].present, false);
            assert_eq!(
                ht.entries[2],
                RHFrame {
                    dib: 0,
                    data: RHData { key: 2, value: 1 },
                    present: true
                }
            );
            assert_eq!(ht.entries[3].present, false);
            assert_eq!(ht.entries[4].present, false);

            assert!(matches!(ht.get(0), AccessRes::DoesntExist));
            assert!(matches!(ht.get(5), AccessRes::DoesntExist));
            assert!(matches!(ht.get(1), AccessRes::DoesntExist));
            assert!(matches!(ht.get(11), AccessRes::DoesntExist));
            assert_eq!(ht.get_expect(2), 1);
        }

        assert!(matches!(ht.delete(2), AccessRes::Success(1)));

        {
            // [ _, _, _, _, _ ]
            assert_eq!(ht.entries[0].present, false);
            assert_eq!(ht.entries[1].present, false);
            assert_eq!(ht.entries[2].present, false);
            assert_eq!(ht.entries[3].present, false);
            assert_eq!(ht.entries[4].present, false);

            assert!(matches!(ht.get(0), AccessRes::DoesntExist));
            assert!(matches!(ht.get(5), AccessRes::DoesntExist));
            assert!(matches!(ht.get(1), AccessRes::DoesntExist));
            assert!(matches!(ht.get(11), AccessRes::DoesntExist));
            assert!(matches!(ht.get(2), AccessRes::DoesntExist));
        }
    }
}
