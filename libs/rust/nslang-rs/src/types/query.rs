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

    // Variable operations
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
}
