pub mod connection;
pub mod connection_state_machine;
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
