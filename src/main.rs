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
pub mod error;
pub mod numstore;
pub mod polling_server;
pub mod protocol;
pub mod robin_hood_ht;

#[cfg(test)]
pub mod test {
    use std::ffi::{CString, c_char, c_int, c_uint};

    unsafe extern "C" {
        #[cfg(feature = "unit_tests")]
        fn run_unit_tests(seed: c_int, filter: *const c_char) -> c_int;

        #[cfg(feature = "irwr_tests")]
        fn irwr_swarm_test(dbname: *const c_char, timeout_seconds: c_int, seed: c_uint);

        #[cfg(feature = "cgd_tests")]
        fn cgd_swarm_test(dbname: *const c_char, timeout_seconds: c_int, seed: c_uint);
    }

    #[test]
    #[cfg(feature = "unit_tests")]
    fn c_unit_tests() {
        let filter_str = CString::new("").expect("Failed to create CString");
        let ret = unsafe { run_unit_tests(1234, filter_str.as_ptr()) };
        assert_eq!(ret, 0);
    }

    #[test]
    #[cfg(feature = "irwr_tests")]
    fn c_irwr_swarm_test() {
        let dbname = CString::new("irwr_swarm_test").expect("Failed to create CString");
        unsafe {
            irwr_swarm_test(dbname.as_ptr(), 2, 1234);
        }
    }

    #[test]
    #[cfg(feature = "cgd_tests")]
    fn c_cgd_swarm_test() {
        let dbname = CString::new("cgd_swarm_test").expect("Failed to create CString");
        unsafe {
            cgd_swarm_test(dbname.as_ptr(), 2, 1234);
        }
    }
}

fn main() {
    println!("Hello world");
}
