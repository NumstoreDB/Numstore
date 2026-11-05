//! The generated module is re-exported so `apps/nsclient` builds its client
//! stubs from here rather than compiling the .proto a second time.

pub mod proto {
    tonic::include_proto!("numstore.v1");
}

pub mod hooks;

use std::pin::Pin;

use futures::{Stream, StreamExt};
use proto::num_store_server::NumStore;
use proto::{
    execute_response, get_response, pull_response, push_request, push_response, ExecuteRequest,
    ExecuteResponse, GetRequest, GetResponse, PullRequest, PullResponse, PushRequest, PushResponse,
};
use tonic::{Request, Response, Status, Streaming};

use hooks::HookError;

fn error(e: HookError) -> proto::Error {
    proto::Error {
        code: e.0,
        message: e.1,
    }
}

#[derive(Debug, Default)]
pub struct NumStoreService;

type ResponseStream<T> = Pin<Box<dyn Stream<Item = Result<T, Status>> + Send>>;

#[tonic::async_trait]
impl NumStore for NumStoreService {
    type PushStream = ResponseStream<PushResponse>;
    type PullStream = ResponseStream<PullResponse>;

    /// Every inbound message gets an ack. After an Error the stream ends.
    async fn push(
        &self,
        request: Request<Streaming<PushRequest>>,
    ) -> Result<Response<Self::PushStream>, Status> {
        // `None` means the query has not arrived yet.
        let init = (request.into_inner(), None::<hooks::Push>, false);

        let out = futures::stream::unfold(init, |(mut inbound, mut push, done)| async move {
            if done {
                return None;
            }

            let msg = match inbound.next().await {
                Some(Ok(m)) => m,
                Some(Err(e)) => return Some((Err(e), (inbound, push, true))),
                None => return None,
            };

            let result = match (msg.msg, push.is_some()) {
                (Some(push_request::Msg::Query(q)), false) => {
                    hooks::check_verb(&q, &["insert", "write"])
                        .and_then(|()| hooks::Push::new(&q))
                        .map(|p| push = Some(p))
                }
                (Some(push_request::Msg::Data(d)), true) => push.as_mut().unwrap().take(d.len()),
                (Some(push_request::Msg::Query(_)), true) => {
                    Err((hooks::ERR_BAD_QUERY, "query already sent".to_string()))
                }
                (Some(push_request::Msg::Data(_)), false) => Err((
                    hooks::ERR_BAD_QUERY,
                    "first message must be the query".to_string(),
                )),
                (None, _) => Err((hooks::ERR_BAD_QUERY, "empty message".to_string())),
            };

            let (reply, stop) = match result {
                Ok(()) => (
                    PushResponse {
                        msg: Some(push_response::Msg::Success(proto::Success {})),
                    },
                    false,
                ),
                Err(e) => (
                    PushResponse {
                        msg: Some(push_response::Msg::Error(error(e))),
                    },
                    true,
                ),
            };

            Some((Ok(reply), (inbound, push, stop)))
        });

        Ok(Response::new(Box::pin(out)))
    }

    /// unfold calls the reader once per chunk the transport accepts, so HTTP/2
    /// flow control is the only backpressure needed.
    async fn pull(
        &self,
        request: Request<PullRequest>,
    ) -> Result<Response<Self::PullStream>, Status> {
        let query = request.into_inner().query;

        if let Err(e) = hooks::check_verb(&query, &["read", "remove"]) {
            let out = futures::stream::once(async move {
                Ok(PullResponse {
                    msg: Some(pull_response::Msg::Error(error(e))),
                })
            });
            return Ok(Response::new(Box::pin(out)));
        }

        // The reader is the state, so dropping the stream drops it.
        let out = futures::stream::unfold(Some(hooks::Reader::new(&query)), |state| async move {
            let mut reader = state?;
            let chunk = reader.read(hooks::CHUNK);
            if chunk.is_empty() {
                return None;
            }
            Some((
                Ok(PullResponse {
                    msg: Some(pull_response::Msg::Data(chunk)),
                }),
                Some(reader),
            ))
        });

        Ok(Response::new(Box::pin(out)))
    }

    async fn execute(
        &self,
        request: Request<ExecuteRequest>,
    ) -> Result<Response<ExecuteResponse>, Status> {
        let query = request.into_inner().query;

        let msg = match hooks::check_verb(&query, &["create", "delete", "remove"])
            .and_then(|()| hooks::execute(&query))
        {
            Ok(()) => execute_response::Msg::Success(proto::Success {}),
            Err(e) => execute_response::Msg::Error(error(e)),
        };

        Ok(Response::new(ExecuteResponse { msg: Some(msg) }))
    }

    async fn get(&self, request: Request<GetRequest>) -> Result<Response<GetResponse>, Status> {
        let query = request.into_inner().query;

        let msg = match hooks::verb(&query) {
            Some(_) => get_response::Msg::Var(hooks::get(&query)),
            None => {
                get_response::Msg::Error(error((hooks::ERR_BAD_QUERY, "empty query".to_string())))
            }
        };

        Ok(Response::new(GetResponse { msg: Some(msg) }))
    }
}
