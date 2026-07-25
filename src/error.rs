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
use std::{io, net};

#[derive(Debug)]
pub enum IOErrorData {
    AddrParseError(net::AddrParseError),
    IOError(io::Error),
}

#[derive(Debug)]
pub enum Error {
    NetworkTimeout(String),
    IncompleteClientMessage,
    IncommingMessageTooLong,
    ClientRecievedInvalidServerMessage,
    IOError(IOErrorData),
}
impl From<io::Error> for Error {
    fn from(value: io::Error) -> Self {
        Error::IOError(IOErrorData::IOError(value))
    }
}

impl From<net::AddrParseError> for Error {
    fn from(value: net::AddrParseError) -> Self {
        Error::IOError(IOErrorData::AddrParseError(value))
    }
}
