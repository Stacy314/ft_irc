# Channel Subsystem — Implementation Plan

**Owner:** Team Member 3
**Scope:** `Channel` class, broadcasting, channel permissions, operator commands
(`KICK`, `INVITE`, `TOPIC`, `MODE` with `+i/-i`, `+t/-t`, `+k/-k`, `+o/-o`, `+l/-l`)
**Standard:** C++98 (`-Wall -Wextra -Werror -std=c++98`)

---

## 1. Locked design decisions

These are settled. Changing any of them requires updating every dependent item below.

| Decision | Rationale |
|---|---|
| Members stored as `std::map<Client*, memberInfo>` | Pointer identity is stable; nicknames are not (`NICK` may change at any time) |
| Operator status is a field of `memberInfo`, not a separate container | Makes "an operator is always a member" structurally impossible to violate |
| Join order tracked by a monotonic per-channel counter | Container size is not usable: it repeats after departures |
| Invite list stored as `std::set<std::string>` (nicknames) | Invited users are not members; storing pointers would couple the list to object lifetime |
| `userLimit == 0` means "no limit" | Avoids a sentinel that collides with `size_t` comparisons |
| `Channel` is non-copyable; default construction is forbidden | A channel without a name is not a valid entity |
| `ChannelResult` is an `enum` with `SUCCESS == 0` | Callers may test `if (result)` for failure |
| Class invariant: a non-empty channel always has at least one operator | Enforced by the class, not by callers |

---

## 2. Implementation sequence

Each phase depends on the previous one. Do not start a phase before its
predecessor compiles and behaves as specified.

### Phase 1 — Membership integrity

Establishes the invariants everything else relies on.

1. `removeMember`
   - Remove the member record.
   - If the channel is now empty, report this upward; stop.
   - Otherwise, if no operators remain, promote the member with the lowest
     join counter.
2. Private helper: locate the member with the lowest join counter.
3. `setOperator` — add an actor parameter and verify the actor's privileges.
   Currently any member can grant themselves operator status.

**Done when:** membership can be added and removed in any order without ever
leaving a non-empty channel without an operator.

### Phase 2 — Uniform permission handling

4. Extract the repeated "is a member / is an operator" block into a private
   helper returning `ChannelResult`. It is currently duplicated six times.
5. Normalise parameter order so the acting client occupies the same position
   in every mutator.
6. `INVITE` requires operator status only when the channel is invite-only.
7. `INVITE` must reject a target that is already a member.

**Done when:** every state-changing method begins with a single call to the
permission helper, and no method bypasses it.

### Phase 3 — Topic

8. `setTopic` — declared but not yet implemented; honours `+t`.
9. `getTopic`.
10. Distinguish "no topic set" from "topic set to an empty string";
    the two map to different numeric replies (`331` vs `332`).

### Phase 4 — Modes

11. `hasKey`, `isFull`.
12. Explicit removal of the key (`-k`) and the limit (`-l`).
13. Assemble the current mode string for reply `324`.
14. Remove the duplicated key check between `isValidKey` and `addMember`.

### Phase 5 — Kick

15. Optional kick reason, defaulting to the actor's nickname.
16. Define behaviour when an operator kicks themselves.

### Phase 6 — Broadcasting

The largest remaining item, and the integration seam with the socket layer.
Until this phase is complete, no state change is visible to any client.

17. `broadcast(message, excluded)` — the single point through which the channel
    emits anything. The delivery mechanism itself is deliberately isolated here
    so it can be replaced once the team settles on one.
18. `getName`.
19. Nickname list for `353` — must return a copy, never a reference to the
    internal container.
20. Operator promotion performed in Phase 1 must emit `MODE +o`; clients cannot
    infer it otherwise.

**Ordering rule:** notifications are broadcast *before* the member is removed,
so that the departing or kicked client receives them.

---

## 3. Cross-cutting concerns

- **Case insensitivity.** Nicknames and channel names compare case-insensitively.
  RFC 1459 additionally treats `[`, `]` and `\` as the lowercase forms of
  `{`, `}` and `|`. A plain `std::string` comparison does not implement this.
- **Null contract.** Either validate incoming pointers or document that callers
  must not pass null. Both are acceptable; silence is not.
- **Message ordering.** See the ordering rule in Phase 6.

---

## 4. Integration contract

### Required from `Client`

- nickname, username, hostname, file descriptor
- a formatted prefix of the form `nick!user@host`

### Required from `Server`

- resolve a nickname to a `Client`
- create and destroy channels
- purge a disconnecting client from every channel, including invite lists,
  before destroying the `Client` object

### Provided to the rest of the team

- `Channel` instances must be stored as pointers. The class is non-copyable,
  so `std::map<std::string, Channel>` will not compile.
- `ChannelResult` values are semantic, not protocol numerics. Translation to
  RFC reply codes belongs to the command dispatcher.

### Known blockers

- `Client.cpp` is not listed in the Makefile; `getNick()` will not link.
- `Client` has no nickname field — only a descriptor and an address.

---

## 5. Numeric replies

Produced by the dispatcher, derived from `ChannelResult`.

| Code | Name | Condition |
|---|---|---|
| 324 | RPL_CHANNELMODEIS | `MODE` query with no arguments |
| 331 | RPL_NOTOPIC | `TOPIC` query, no topic set |
| 332 | RPL_TOPIC | `TOPIC` query, topic present |
| 341 | RPL_INVITING | `INVITE` accepted |
| 353 / 366 | RPL_NAMREPLY / RPL_ENDOFNAMES | member list after join |
| 401 | ERR_NOSUCHNICK | target nickname unknown to the server |
| 403 | ERR_NOSUCHCHANNEL | channel does not exist |
| 441 | ERR_USERNOTINCHANNEL | target is not on the channel |
| 442 | ERR_NOTONCHANNEL | actor is not on the channel |
| 443 | ERR_USERONCHANNEL | invite target is already on the channel |
| 461 | ERR_NEEDMOREPARAMS | missing arguments |
| 467 | ERR_KEYSET | `+k` while a key is already set |
| 471 | ERR_CHANNELISFULL | join blocked by `+l` |
| 472 | ERR_UNKNOWNMODE | unrecognised mode character |
| 473 | ERR_INVITEONLYCHAN | join blocked by `+i` |
| 475 | ERR_BADCHANNELKEY | join blocked by `+k` |
| 482 | ERR_CHANOPRIVSNEEDED | actor lacks operator status |

Successful state changes carry no numeric reply. They are echoed to the channel
as prefixed commands (`KICK`, `MODE`, `TOPIC`, `INVITE`).

Note: parameter order for `341` differs between RFC 1459 and RFC 2812.
Pick one revision and apply it consistently.
