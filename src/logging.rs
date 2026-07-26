use std::fmt::Arguments;

pub trait Logger {
    fn log_info(&self, args: Arguments);
    fn log_debug(&self, args: Arguments);
    fn log_warn(&self, args: Arguments);
    fn log_error(&self, args: Arguments);
}

pub struct ConsoleLogger;

impl ConsoleLogger {
    pub fn new() -> Self {
        ConsoleLogger {}
    }
}

impl Logger for ConsoleLogger {
    fn log_info(&self, args: Arguments) {
        println!("INFO: {}", args);
    }
    fn log_debug(&self, args: Arguments) {
        println!("DEBUG: {}", args);
    }
    fn log_warn(&self, args: Arguments) {
        println!("WARN: {}", args);
    }
    fn log_error(&self, args: Arguments) {
        println!("ERROR: {}", args);
    }
}

#[macro_export]
macro_rules! log_info {
    ($logger:expr, $($arg:tt)*) => {
        $logger.log_info(format_args!($($arg)*))
    };
}

#[macro_export]
macro_rules! log_debug {
    ($logger:expr, $($arg:tt)*) => {
        $logger.log_debug(format_args!($($arg)*))
    };
}

#[macro_export]
macro_rules! log_warn {
    ($logger:expr, $($arg:tt)*) => {
        $logger.log_warn(format_args!($($arg)*))
    };
}

#[macro_export]
macro_rules! log_error {
    ($logger:expr, $($arg:tt)*) => {
        $logger.log_error(format_args!($($arg)*))
    };
}
