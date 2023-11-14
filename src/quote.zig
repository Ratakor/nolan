const std = @import("std");
const cURL = @cImport({
    @cInclude("curl/curl.h");
});

const KaamelottResponse = struct {
    status: u32,
    citation: struct {
        citation: []const u8,
        infos: struct {
            auteur: []const u8,
            acteur: []const u8,
            personnage: []const u8,
            saison: []const u8,
            episode: []const u8,
        },
    },

    const api_url = "https://kaamelott.chaudie.re/api/random";
};

const Oss117Response = struct {
    sentence: []const u8,
    character: struct {
        name: []const u8,
        slug: []const u8,
    },

    const api_url = "https://api.oss117quotes.xyz/v1/random";
};

const ChuckNorrisResponse = struct {
    categories: [][]const u8,
    created_at: []const u8,
    icon_url: []const u8,
    id: []const u8,
    updated_at: []const u8,
    url: []const u8,
    value: []const u8,

    const api_url = "https://api.chucknorris.io/jokes/random";
};

fn writeToArrayListCallback(data: *anyopaque, size: c_uint, nmemb: c_uint, user_data: *anyopaque) callconv(.C) c_uint {
    var buffer: *std.ArrayList(u8) = @alignCast(@ptrCast(user_data));
    var typed_data: [*]u8 = @ptrCast(data);
    buffer.appendSlice(typed_data[0 .. nmemb * size]) catch return 0;
    return nmemb * size;
}

fn writeQuote(comptime Response: type, buf: []u8) !usize {
    var arena = std.heap.ArenaAllocator.init(std.heap.c_allocator);
    defer arena.deinit();
    const allocator = arena.allocator();

    const handle = cURL.curl_easy_init() orelse return error.CURLHandleInitFailed;
    defer cURL.curl_easy_cleanup(handle);

    var buffer = std.ArrayList(u8).init(allocator);
    // defer buffer.deinit();

    if (cURL.curl_easy_setopt(handle, cURL.CURLOPT_URL, Response.api_url) != cURL.CURLE_OK)
        return error.CouldNotSetURL;
    if (cURL.curl_easy_setopt(handle, cURL.CURLOPT_WRITEFUNCTION, writeToArrayListCallback) != cURL.CURLE_OK)
        return error.CouldNotSetWriteCallback;
    if (cURL.curl_easy_setopt(handle, cURL.CURLOPT_WRITEDATA, &buffer) != cURL.CURLE_OK)
        return error.CouldNotSetWriteCallback;

    if (cURL.curl_easy_perform(handle) != cURL.CURLE_OK)
        return error.FailedToPerformRequest;

    const response = try std.json.parseFromSliceLeaky(Response, allocator, buffer.items, .{});

    return (switch (Response) {
        KaamelottResponse => try std.fmt.bufPrintZ(buf, "{s}\n> {s} (Kaamelott)\n\n", .{
            response.citation.citation,
            response.citation.infos.personnage,
        }),
        Oss117Response => try std.fmt.bufPrintZ(buf, "{s}\n> {s} (OSS 117)\n\n", .{
            response.sentence,
            response.character.name,
        }),
        ChuckNorrisResponse => try std.fmt.bufPrintZ(buf, "{s} (Chuck Norris)\n\n", .{response.value}),
        else => unreachable,
    }).len;
}

var count = std.atomic.Atomic(usize).init(0);

export fn write_quote(buffer: [*]u8, size: usize) usize {
    return switch (count.fetchAdd(1, .AcqRel)) {
        0 => writeQuote(KaamelottResponse, buffer[0..size]),
        1 => writeQuote(Oss117Response, buffer[0..size]),
        else => blk: {
            count.store(0, .Release);
            break :blk writeQuote(ChuckNorrisResponse, buffer[0..size]);
        },
    } catch |err| blk: {
        std.log.warn("write_quote: {}", .{err});
        buffer[0] = 0;
        break :blk 0;
    };
}
