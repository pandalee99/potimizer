import java.util.concurrent.*;
import java.util.function.Function;
import java.util.*;

public class RequestAggregator<T, R> {
    private final Map<T, CompletableFuture<R>> cache = new ConcurrentHashMap<>();
    private final Function<List<T>, List<R>> batchFunction;

    public RequestAggregator(Function<List<T>, List<R>> batchFunction) {
        this.batchFunction = batchFunction;
    }

    public CompletableFuture<R> sendRequest(T request) {
        // 尝试从缓存中获取已有的请求
        CompletableFuture<R> responseFuture = cache.get(request);
        if (responseFuture != null) {
            return responseFuture;
        }

        // 创建一个新的CompletableFuture
        CompletableFuture<R> newResponseFuture = new CompletableFuture<>();
        cache.put(request, newResponseFuture);

        // 在这里可以设置一个定时任务，比如每隔100毫秒检查一次
        // 如果有多个请求，就批量发送
        // 这里简化处理，直接调用batchProcessRequests
        batchProcessRequests();

        return newResponseFuture;
    }

    private synchronized void batchProcessRequests() {
        // 获取所有的请求
        Set<T> requests = cache.keySet();
        if (requests.isEmpty()) {
            return;
        }

        // 将请求转换为列表，并发送批量请求
        List<T> requestList = new ArrayList<>(requests);
        List<R> responses = batchFunction.apply(requestList);

        // 将结果分发到对应的CompletableFuture
        for (int i = 0; i < requestList.size(); i++) {
            T request = requestList.get(i);
            R response = responses.get(i);
            CompletableFuture<R> responseFuture = cache.remove(request);
            if (responseFuture != null) {
                responseFuture.complete(response);
            }
        }
    }
}

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
