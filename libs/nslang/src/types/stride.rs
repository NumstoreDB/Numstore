use std::fmt;
use std::num::NonZeroI64;

use crate::error::Result;
use crate::ffi::Arena;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum UserStride {
    /// `[start]`: no colon, a single index.
    Index(i64),
    /// `[start:stop:step]`: a colon was present; each number may be omitted.
    Slice {
        start: Option<i64>,
        stop: Option<i64>,
        step: Option<NonZeroI64>,
    },
}

struct MultiUserStride {
    strides: [UserStride],
}

impl UserStride {
    /// `[:]`
    pub const FULL: Self = Self::Slice {
        start: None,
        stop: None,
        step: None,
    };

    fn to_c(&self) -> CUserStride {
        match *self {
            UserStride::Index(i) => CUserStride {
                start: i,
                step: 0,
                stop: 0,
                present: START_PRESENT,
            },
            UserStride::Slice { start, stop, step } => CUserStride {
                start: start.unwrap_or(0),
                step: step.map_or(0, NonZeroI64::get),
                stop: stop.unwrap_or(0),
                present: COLON_PRESENT
                    | if start.is_some() { START_PRESENT } else { 0 }
                    | if stop.is_some() { STOP_PRESENT } else { 0 }
                    | if step.is_some() { STEP_PRESENT } else { 0 },
            },
        }
    }
}

impl MultiUserStride {
    fn to_c(&self, a: &Arena<'_>) -> Result<*mut CUserStride> {
        let lowered: Vec<CUserStride> = self.strides.iter().map(UserStride::to_c).collect();
        a.slice(&lowered)
    }
}

impl fmt::Display for UserStride {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            Self::Index(i) => write!(f, "[{i}]"),
            Self::Slice { start, stop, step } => {
                f.write_str("[")?;
                if let Some(v) = start {
                    write!(f, "{v}")?;
                }
                f.write_str(":")?;
                if let Some(v) = stop {
                    write!(f, "{v}")?;
                }
                if let Some(v) = step {
                    write!(f, ":{v}")?;
                }
                f.write_str("]")
            }
        }
    }
}

///////////////////////// Stride C FFI

pub const START_PRESENT: u32 = 1 << 0;
pub const STEP_PRESENT: u32 = 1 << 1;
pub const STOP_PRESENT: u32 = 1 << 2;
pub const COLON_PRESENT: u32 = 1 << 3;

// struct user_stride (core/ns_stride.h)
#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct CUserStride {
    pub start: i64,
    pub step: i64,
    pub stop: i64,
    pub present: u32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CMultiUserStride {
    pub strides: *mut CUserStride,
    pub len: u32,
}
