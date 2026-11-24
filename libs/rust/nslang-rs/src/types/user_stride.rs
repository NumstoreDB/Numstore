use std::fmt;
use std::num::NonZeroI64;

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

impl UserStride {
    /// `[:]`
    pub const FULL: Self = Self::Slice {
        start: None,
        stop: None,
        step: None,
    };
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
