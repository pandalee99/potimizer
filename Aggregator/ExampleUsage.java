package Aggregator;

import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.function.Function;

// 使用示例
public class ExampleUsage {
    public static void main(String[] args) {
        // 假设我们有一个批量请求的函数
        Function<List<String>, List<String>> batchFunction = (requests) -> {
            // 发送批量请求到后端，并获取响应
            // 这里只是一个示例，实际中应该是调用后端服务
            return requests.stream().map(request -> "Response for " + request).toList();
        };

        RequestAggregator<String, String> aggregator = new RequestAggregator<>(batchFunction);

        // 模拟多个请求
        CompletableFuture<String> response1 = aggregator.sendRequest("request1");
        CompletableFuture<String> response2 = aggregator.sendRequest("request2");

        // 处理响应
        response1.thenAccept(response -> System.out.println("Response 1: " + response));
        response2.thenAccept(response -> System.out.println("Response 2: " + response));
    }
}