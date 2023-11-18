const std = @import("std");
const cURL = @cImport({
    @cInclude("curl/curl.h");
});

const KaamelottApi = struct {
    const Response = struct {
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
    };

    const url = "https://kaamelott.chaudie.re/api/random";
};

const Oss117Api = struct {
    const Response = struct {
        sentence: []const u8,
        character: struct {
            name: []const u8,
            slug: []const u8,
        },
    };

    const url = "https://api.oss117quotes.xyz/v1/random";
};

const SouthParkApi = struct {
    const Response = []struct {
        quote: []const u8,
        character: []const u8,
    };

    const url = "https://southparkquotes.onrender.com/v1/quotes";
};

fn writeToArrayListCallback(data: *anyopaque, size: c_uint, nmemb: c_uint, user_data: *anyopaque) callconv(.C) c_uint {
    var buffer: *std.ArrayList(u8) = @alignCast(@ptrCast(user_data));
    var typed_data: [*]u8 = @ptrCast(data);
    buffer.appendSlice(typed_data[0 .. nmemb * size]) catch return 0;
    return nmemb * size;
}

fn writeQuote(comptime Api: type, buf: []u8) !usize {
    var arena = std.heap.ArenaAllocator.init(std.heap.c_allocator);
    defer arena.deinit();
    const allocator = arena.allocator();

    const handle = cURL.curl_easy_init() orelse return error.CURLHandleInitFailed;
    defer cURL.curl_easy_cleanup(handle);

    var buffer = std.ArrayList(u8).init(allocator);
    // defer buffer.deinit();

    if (cURL.curl_easy_setopt(handle, cURL.CURLOPT_URL, Api.url) != cURL.CURLE_OK)
        return error.CouldNotSetURL;
    if (cURL.curl_easy_setopt(handle, cURL.CURLOPT_WRITEFUNCTION, writeToArrayListCallback) != cURL.CURLE_OK)
        return error.CouldNotSetWriteCallback;
    if (cURL.curl_easy_setopt(handle, cURL.CURLOPT_WRITEDATA, &buffer) != cURL.CURLE_OK)
        return error.CouldNotSetWriteCallback;

    if (cURL.curl_easy_perform(handle) != cURL.CURLE_OK)
        return error.FailedToPerformRequest;

    const response = try std.json.parseFromSliceLeaky(Api.Response, allocator, buffer.items, .{});

    return (switch (Api) {
        KaamelottApi => try std.fmt.bufPrintZ(buf, "{s}\n> {s} (Kaamelott)\n\n", .{
            response.citation.citation,
            response.citation.infos.personnage,
        }),
        Oss117Api => try std.fmt.bufPrintZ(buf, "{s}\n> {s} (OSS 117)\n\n", .{
            response.sentence,
            response.character.name,
        }),
        SouthParkApi => try std.fmt.bufPrintZ(buf, "{s}\n> {s} (South Park)\n\n", .{ response[0].quote, response[0].character }),
        else => unreachable,
    }).len;
}

var count = std.atomic.Atomic(usize).init(0);

export fn write_quote(buffer: [*]u8, size: usize) usize {
    return switch (count.fetchAdd(1, .AcqRel)) {
        0 => writeQuote(KaamelottApi, buffer[0..size]),
        1 => writeQuote(Oss117Api, buffer[0..size]),
        else => blk: {
            count.store(0, .Release);
            break :blk writeQuote(SouthParkApi, buffer[0..size]);
        },
    } catch |err| blk: {
        std.log.warn("{s}: {}", .{@src().fn_name, err});
        buffer[0] = 0;
        break :blk 0;
    };
}
