use std::mem;

use crate::error::Result;
use crate::ffi::{Arena, CStr32, CType, CUserStride};
use crate::types::types::Type;
use crate::types::user_stride::UserStride;

#[derive(Debug, Clone, PartialEq)]
pub enum Query<'src> {
    Read {
        name: &'src str,
        ustr: UserStride,
    },

    Write {
        name: &'src str,
        ustr: UserStride,
    },

    Insert {
        name: &'src str,
        ofst: i64,
        len: u64,
    },

    Remove {
        name: &'src str,
        ustr: UserStride,
    },

    Create {
        name: &'src str,
        ty: Type<'src>,
    },

    Delete {
        name: &'src str,
        if_exists: bool,
    },

    Get {
        name: &'src str,
        if_exists: bool,
    },

    // Meta operations
    Exit,

    Help {
        command: Option<QueryKind>,
    },
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum QueryKind {
    Read,
    Write,
    Insert,
    Remove,
    Create,
    Delete,
    Get,
    Exit,
    Help,
}

impl<'src> Query<'src> {
    pub fn kind(&self) -> QueryKind {
        match self {
            Query::Read { .. } => QueryKind::Read,
            Query::Write { .. } => QueryKind::Write,
            Query::Insert { .. } => QueryKind::Insert,
            Query::Remove { .. } => QueryKind::Remove,
            Query::Create { .. } => QueryKind::Create,
            Query::Delete { .. } => QueryKind::Delete,
            Query::Get { .. } => QueryKind::Get,
            Query::Exit => QueryKind::Exit,
            Query::Help { .. } => QueryKind::Help,
        }
    }

    pub fn vname(&self) -> Option<&'src str> {
        match *self {
            Query::Read { name, .. }
            | Query::Write { name, .. }
            | Query::Insert { name, .. }
            | Query::Remove { name, .. }
            | Query::Create { name, .. }
            | Query::Delete { name, .. }
            | Query::Get { name, .. } => Some(name),
            Query::Exit | Query::Help { .. } => None,
        }
    }

    pub fn ustr(&self) -> Option<&UserStride> {
        match self {
            Query::Read { ustr, .. } | Query::Write { ustr, .. } | Query::Remove { ustr, .. } => {
                Some(ustr)
            }
            _ => None,
        }
    }

    fn to_c(&self, a: &Arena<'_>) -> Result<CQuery> {
        // Zero out a new CQuery
        let mut out: CQuery = unsafe { mem::zeroed() };

        // Get the tag
        out.tag = query_kind_tag(self.kind());

        match self {
            Query::Read { name, ustr }
            | Query::Write { name, ustr }
            | Query::Remove { name, ustr } => {
                out.u.strided = CStridedQuery {
                    name: a.string(name)?,
                    ustr: lower_stride(ustr),
                };
            }
            Query::Insert { name, ofst, len } => {
                out.u.insert = CInsertQuery {
                    name: a.string(name)?,
                    ofst: *ofst,
                    len: *len,
                };
            }
            Query::Create { name, ty } => {
                ty.validate()?;
                out.u.create = CCreateQuery {
                    name: a.string(name)?,
                    ty: lower_type(a, ty)?,
                };
            }
            Query::Delete { name, if_exists } | Query::Get { name, if_exists } => {
                out.u.named = CNamedQuery {
                    name: a.string(name)?,
                    if_exists: (*if_exists).into(),
                };
            }
            Query::Exit => {}
            Query::Help { command } => {
                out.u.help = CHelpQuery {
                    has_command: command.is_some().into(),
                    command: command.map_or(0, query_kind_tag),
                };
            }
        }

        Ok(out)
    }
}

///////////////////////// Query C FFI

// C representations of each flag
pub const QT_READ: u32 = 0;
pub const QT_WRITE: u32 = 1;
pub const QT_INSERT: u32 = 2;
pub const QT_REMOVE: u32 = 3;
pub const QT_CREATE: u32 = 4;
pub const QT_DELETE: u32 = 5;
pub const QT_GET: u32 = 6;
pub const QT_EXIT: u32 = 7;
pub const QT_HELP: u32 = 8;

// convert a QueryKind into a C Query tag
fn query_kind_tag(kind: QueryKind) -> u32 {
    match kind {
        QueryKind::Read => QT_READ,
        QueryKind::Write => QT_WRITE,
        QueryKind::Insert => QT_INSERT,
        QueryKind::Remove => QT_REMOVE,
        QueryKind::Create => QT_CREATE,
        QueryKind::Delete => QT_DELETE,
        QueryKind::Get => QT_GET,
        QueryKind::Exit => QT_EXIT,
        QueryKind::Help => QT_HELP,
    }
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CQuery {
    pub tag: u32,
    pub u: CQueryBody,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub union CQueryBody {
    // read, write and remove
    pub strided: CStridedQuery,
    pub insert: CInsertQuery,
    pub create: CCreateQuery,
    // delete and get
    pub named: CNamedQuery,
    pub help: CHelpQuery,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CStridedQuery {
    pub name: CStr32,
    pub ustr: CUserStride,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CInsertQuery {
    pub name: CStr32,
    pub ofst: i64,
    pub len: u64,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CCreateQuery {
    pub name: CStr32,
    pub ty: CType,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CNamedQuery {
    pub name: CStr32,
    pub if_exists: u8,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CHelpQuery {
    pub has_command: u8,
    pub command: u32,
}
