use nsserver::proto::num_store_client::NumStoreClient;
use nsserver::proto::{
    execute_response, get_response, pull_response, push_request, push_response, ExecuteRequest,
    GetRequest, PullRequest, PushRequest,
};
use tokio_stream::StreamExt;

fn query(q: &str) -> PushRequest {
    PushRequest {
        msg: Some(push_request::Msg::Query(q.to_string())),
    }
}

fn chunk(n: usize) -> PushRequest {
    PushRequest {
        msg: Some(push_request::Msg::Data(vec![0u8; n])),
    }
}

fn show_push(label: &str, r: push_response::Msg) {
    match r {
        push_response::Msg::Success(_) => println!("  {label}: Success"),
        push_response::Msg::Error(e) => println!("  {label}: Error {} {}", e.code, e.message),
    }
}

async fn push(
    client: &mut NumStoreClient<tonic::transport::Channel>,
    label: &str,
    msgs: Vec<PushRequest>,
) {
    println!("{label}");
    let mut acks = client
        .push(tokio_stream::iter(msgs))
        .await
        .unwrap()
        .into_inner();
    let mut i = 0;
    while let Some(ack) = acks.next().await {
        show_push(&format!("msg {i}"), ack.unwrap().msg.unwrap());
        i += 1;
    }
}

#[tokio::main]
async fn main() {
    let mut client = NumStoreClient::connect("http://127.0.0.1:50051")
        .await
        .unwrap();

    push(
        &mut client,
        "1. push \"insert foo 0 300\" + 3x100 bytes",
        vec![
            query("insert foo 0 300"),
            chunk(100),
            chunk(100),
            chunk(100),
        ],
    )
    .await;

    println!("2. pull \"read foo[0:10]\"");
    let mut stream = client
        .pull(PullRequest {
            query: "read foo[0:10]".into(),
        })
        .await
        .unwrap()
        .into_inner();
    let mut total = 0usize;
    while let Some(msg) = stream.next().await {
        match msg.unwrap().msg.unwrap() {
            pull_response::Msg::Data(d) => total += d.len(),
            pull_response::Msg::Error(e) => println!("  Error {} {}", e.code, e.message),
        }
    }
    println!("  received {total} bytes");

    println!("3. execute \"create foo\"");
    match client
        .execute(ExecuteRequest {
            query: "create foo".into(),
        })
        .await
        .unwrap()
        .into_inner()
        .msg
        .unwrap()
    {
        execute_response::Msg::Success(_) => println!("  Success"),
        execute_response::Msg::Error(e) => println!("  Error {} {}", e.code, e.message),
    }

    println!("4. get \"get foo\"");
    match client
        .get(GetRequest {
            query: "get foo".into(),
        })
        .await
        .unwrap()
        .into_inner()
        .msg
        .unwrap()
    {
        get_response::Msg::Var(v) => println!("  var = {:?}", String::from_utf8_lossy(&v)),
        get_response::Msg::Error(e) => println!("  Error {} {}", e.code, e.message),
    }

    push(
        &mut client,
        "5a. push too many bytes (expect Error 2)",
        vec![query("insert foo 0 10"), chunk(100)],
    )
    .await;

    push(
        &mut client,
        "5b. push with a wrong verb (expect Error 1)",
        vec![query("read foo 0 10")],
    )
    .await;
}
