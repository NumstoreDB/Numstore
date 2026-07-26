use crate::error::Error;

impl ProtocolState {
    fn tx_query(&self, query: &str) -> ProtocolState {
        match self {
            ProtocolState::SendingQuery => return ProtocolState::WaitingForServer,
            _ => unreachable!(),
        }

        unreachable!()
    }
}
