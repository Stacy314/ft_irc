#!/usr/bin/env python3
"""
test_ircserv.py — тестовый скрипт для проекта FT_IRC (42 school)

Проверяет сервер по протоколу IRC "снаружи", через сырые TCP-сокеты,
без сторонних IRC-библиотек — так тестер не зависит от их трактовки
протокола и видит именно то, что реально шлёт твой сервер.

ЗАПУСК:
    1) Собери и запусти сервер в отдельном терминале:
         ./ircserv <port> <password>
    2) В другом терминале запусти тесты:
         python3 test_ircserv.py <port> <password> [host]

    По умолчанию host = 127.0.0.1

Скрипт НЕ запускает сервер сам — считает, что он уже поднят,
как и требуется по условиям проекта (тестер подключается снаружи).

СТРУКТУРА:
    - IRCClient: обёртка над сокетом с методами send()/expect()/read_all()
    - TestSuite: набор тестов, сгруппированных по темам
    - stress_test(): нагрузочный тест с N параллельными клиентами
    - main(): парсит аргументы, гоняет тесты, печатает отчёт

Каждый тест — независимая функция, возвращающая (bool, str) —
результат и пояснение. Так падение одного теста не роняет остальные.
"""

import socket
import sys
import time
import threading
import random
import string

# ---------------------------------------------------------------------------
# Настройки
# ---------------------------------------------------------------------------

RECV_TIMEOUT = 2.0       # сколько ждать ответа от сервера на каждый recv()
CONNECT_TIMEOUT = 3.0    # таймаут на установление TCP-соединения

COLOR_GREEN = "\033[92m"
COLOR_RED = "\033[91m"
COLOR_YELLOW = "\033[93m"
COLOR_CYAN = "\033[96m"
COLOR_RESET = "\033[0m"
COLOR_BOLD = "\033[1m"


def ok(msg):
    print(f"  {COLOR_GREEN}[PASS]{COLOR_RESET} {msg}")


def fail(msg):
    print(f"  {COLOR_RED}[FAIL]{COLOR_RESET} {msg}")


def info(msg):
    print(f"  {COLOR_YELLOW}[INFO]{COLOR_RESET} {msg}")


def section(title):
    print(f"\n{COLOR_BOLD}{COLOR_CYAN}=== {title} ==={COLOR_RESET}")


def random_str(n=6):
    return "".join(random.choices(string.ascii_lowercase, k=n))


# ---------------------------------------------------------------------------
# Обёртка клиента
# ---------------------------------------------------------------------------

class IRCClient:
    """Тонкая обёртка над TCP-сокетом для общения с IRC-сервером."""

    def __init__(self, host, port, timeout=CONNECT_TIMEOUT):
        self.sock = socket.create_connection((host, port), timeout=timeout)
        self.sock.settimeout(RECV_TIMEOUT)
        self.buffer = ""

    def send(self, line: str):
        """Отправляет одну IRC-команду (сам добавляет \\r\\n)."""
        if not line.endswith("\r\n"):
            line = line + "\r\n"
        self.sock.sendall(line.encode("utf-8", errors="ignore"))

    def read_line(self, timeout=None):
        """Читает одну строку (до \\r\\n или \\n). Возвращает None по таймауту."""
        if timeout is not None:
            self.sock.settimeout(timeout)
        try:
            while "\n" not in self.buffer:
                chunk = self.sock.recv(4096)
                if not chunk:
                    return None
                self.buffer += chunk.decode("utf-8", errors="ignore")
        except socket.timeout:
            return None
        finally:
            if timeout is not None:
                self.sock.settimeout(RECV_TIMEOUT)

        line, _, rest = self.buffer.partition("\n")
        self.buffer = rest
        return line.rstrip("\r")

    def read_all(self, duration=RECV_TIMEOUT):
        """Собирает все строки, пришедшие за `duration` секунд."""
        lines = []
        deadline = time.time() + duration
        while True:
            remaining = deadline - time.time()
            if remaining <= 0:
                break
            line = self.read_line(timeout=remaining)
            if line is None:
                break
            lines.append(line)
        return lines

    def expect(self, predicate, duration=RECV_TIMEOUT):
        """
        Читает строки, пока predicate(line) не вернёт True, либо пока
        не истечёт `duration`. Возвращает найденную строку или None.
        Все прочитанные строки сохраняются в self.last_batch.
        """
        self.last_batch = []
        deadline = time.time() + duration
        while True:
            remaining = deadline - time.time()
            if remaining <= 0:
                return None
            line = self.read_line(timeout=remaining)
            if line is None:
                return None
            self.last_batch.append(line)
            if predicate(line):
                return line

    def register(self, nick, user, password=None, realname="test"):
        """Полный цикл регистрации: PASS/NICK/USER, ждёт 001."""
        if password is not None:
            self.send(f"PASS {password}")
        self.send(f"NICK {nick}")
        self.send(f"USER {user} 0 * :{realname}")
        return self.expect(lambda l: " 001 " in l or l.split()[1:2] == ["001"])

    def close(self):
        try:
            self.sock.close()
        except OSError:
            pass


def has_code(lines, code):
    """Проверяет, есть ли среди строк численный код ответа IRC (например '332')."""
    for l in lines:
        parts = l.split()
        if len(parts) >= 2 and parts[1] == code:
            return True
    return False


def find_code(lines, code):
    for l in lines:
        parts = l.split()
        if len(parts) >= 2 and parts[1] == code:
            return l
    return None


# ---------------------------------------------------------------------------
# Тесты
# ---------------------------------------------------------------------------

class TestSuite:
    def __init__(self, host, port, password):
        self.host = host
        self.port = port
        self.password = password
        self.results = []  # (name, bool)

    def record(self, name, passed, detail=""):
        self.results.append((name, passed))
        if passed:
            ok(name + (f" — {detail}" if detail else ""))
        else:
            fail(name + (f" — {detail}" if detail else ""))

    def new_client(self):
        return IRCClient(self.host, self.port)

    # ---------------- Подключение и аутентификация ----------------

    def test_tcp_connect(self):
        section("Базовое подключение")
        try:
            c = self.new_client()
            c.close()
            self.record("TCP-соединение устанавливается", True)
        except Exception as e:
            self.record("TCP-соединение устанавливается", False, str(e))

    def test_wrong_password_rejected(self):
        try:
            c = self.new_client()
            c.send(f"PASS {self.password}wrong")
            c.send(f"NICK wrongpass_{random_str()}")
            c.send(f"USER u 0 * :real")
            lines = c.read_all(1.5)
            # Ожидаем ERR_PASSWDMISMATCH (464) либо разрыв соединения
            rejected = has_code(lines, "464")
            still_open = True
            try:
                c.sock.sendall(b"PING :x\r\n")
            except OSError:
                still_open = False
            c.close()
            self.record(
                "Неверный пароль отклоняется (464 или разрыв соединения)",
                rejected or not still_open,
                "получено: " + " | ".join(lines) if lines else "нет ответа",
            )
        except Exception as e:
            self.record("Неверный пароль отклоняется", False, str(e))

    def test_registration_success(self):
        try:
            c = self.new_client()
            welcome = c.register(f"nick_{random_str()}", "user1", self.password)
            self.record(
                "Успешная регистрация (PASS/NICK/USER) -> 001 Welcome",
                welcome is not None,
                welcome or "код 001 не получен: " + " | ".join(c.last_batch),
            )
            self.client_main = c  # переиспользуем дальше
        except Exception as e:
            self.record("Успешная регистрация", False, str(e))
            self.client_main = None

    def test_nick_collision(self):
        try:
            shared_nick = f"dup_{random_str()}"
            c1 = self.new_client()
            c1.register(shared_nick, "user1", self.password)

            c2 = self.new_client()
            c2.send(f"PASS {self.password}")
            c2.send(f"NICK {shared_nick}")
            c2.send(f"USER user2 0 * :real")
            lines = c2.read_all(1.5)
            collided = has_code(lines, "433")  # ERR_NICKNAMEINUSE
            self.record(
                "Занятый ник отклоняется (433 ERR_NICKNAMEINUSE)",
                collided,
                "получено: " + " | ".join(lines) if lines else "нет ответа",
            )
            c1.close()
            c2.close()
        except Exception as e:
            self.record("Занятый ник отклоняется", False, str(e))

    def test_ping_pong(self):
        try:
            c = self.new_client()
            c.register(f"pinger_{random_str()}", "user1", self.password)
            c.send("PING :hello")
            reply = c.expect(lambda l: l.upper().startswith("PONG") or "PONG" in l.upper())
            self.record(
                "Сервер отвечает PONG на PING",
                reply is not None,
                reply or "нет ответа на PING",
            )
            c.close()
        except Exception as e:
            self.record("PING/PONG", False, str(e))

    # ---------------- Каналы ----------------

    def test_join_creates_channel(self):
        section("Каналы: JOIN / PART / TOPIC / KICK")
        try:
            c = self.new_client()
            nick = f"join_{random_str()}"
            c.register(nick, "user1", self.password)
            chan = f"#test_{random_str()}"
            c.send(f"JOIN {chan}")
            lines = c.read_all(1.5)
            # Ожидаем либо подтверждение JOIN (эхо), либо 353 (NAMES) / 366
            joined = any(f"JOIN {chan}" in l or f"JOIN :{chan}" in l for l in lines) or \
                has_code(lines, "353") or has_code(lines, "366")
            self.record(
                "JOIN на новый канал создаёт его и подтверждается",
                joined,
                "получено: " + " | ".join(lines) if lines else "нет ответа",
            )
            self.chan_client = c
            self.chan_name = chan
            self.chan_nick = nick
        except Exception as e:
            self.record("JOIN создаёт канал", False, str(e))
            self.chan_client = None

    def test_second_client_join_sees_first(self):
        if not getattr(self, "chan_client", None):
            self.record("Второй клиент видит первого в канале", False, "пропущено (нет базового клиента)")
            return
        try:
            c2 = self.new_client()
            nick2 = f"join2_{random_str()}"
            c2.register(nick2, "user2", self.password)
            c2.send(f"JOIN {self.chan_name}")
            lines2 = c2.read_all(1.5)
            names_ok = has_code(lines2, "353")

            # первый клиент должен получить уведомление о JOIN второго
            lines1 = self.chan_client.read_all(1.5)
            notified = any(nick2 in l and "JOIN" in l for l in lines1)

            self.record(
                "Второй клиент получает список пользователей (353 NAMES)",
                names_ok,
                "получено: " + " | ".join(lines2) if lines2 else "нет ответа",
            )
            self.record(
                "Первый клиент уведомлён о присоединении второго",
                notified,
                "получено: " + " | ".join(lines1) if lines1 else "нет ответа",
            )
            self.chan_client2 = c2
            self.chan_nick2 = nick2
        except Exception as e:
            self.record("Второй клиент в канале", False, str(e))

    def test_topic(self):
        if not getattr(self, "chan_client", None):
            self.record("TOPIC", False, "пропущено (нет базового клиента)")
            return
        try:
            c = self.chan_client
            new_topic = f"topic_{random_str()}"
            c.send(f"TOPIC {self.chan_name} :{new_topic}")
            lines = c.read_all(1.0)
            c.send(f"TOPIC {self.chan_name}")
            lines2 = c.read_all(1.0)
            has_topic = any(new_topic in l for l in lines + lines2)
            self.record(
                "TOPIC устанавливается и возвращается корректно",
                has_topic,
                "получено: " + " | ".join(lines + lines2) if (lines or lines2) else "нет ответа",
            )
        except Exception as e:
            self.record("TOPIC", False, str(e))

    def test_part(self):
        if not getattr(self, "chan_client2", None):
            self.record("PART", False, "пропущено (нет второго клиента)")
            return
        try:
            c2 = self.chan_client2
            c2.send(f"PART {self.chan_name} :bye")
            lines = c2.read_all(1.5)
            parted = any("PART" in l for l in lines)
            self.record(
                "PART подтверждается сервером",
                parted,
                "получено: " + " | ".join(lines) if lines else "нет ответа",
            )
            c2.close()
        except Exception as e:
            self.record("PART", False, str(e))

    def test_kick(self):
        if not getattr(self, "chan_client", None):
            self.record("KICK", False, "пропущено (нет базового клиента)")
            return
        try:
            c3 = self.new_client()
            nick3 = f"kickme_{random_str()}"
            c3.register(nick3, "user3", self.password)
            c3.send(f"JOIN {self.chan_name}")
            c3.read_all(1.0)

            self.chan_client.send(f"KICK {self.chan_name} {nick3} :test kick")
            lines_kicker = self.chan_client.read_all(1.0)
            lines_kicked = c3.read_all(1.0)

            kicked = any("KICK" in l for l in lines_kicker + lines_kicked)
            self.record(
                "KICK от оператора канала работает",
                kicked,
                "получено (kicker): " + " | ".join(lines_kicker)
                + " | (kicked): " + " | ".join(lines_kicked),
            )
            c3.close()
        except Exception as e:
            self.record("KICK", False, str(e))

    # ---------------- Операторские команды ----------------

    def test_mode_invite_only(self):
        section("Операторские команды: MODE / INVITE")
        if not getattr(self, "chan_client", None):
            self.record("MODE +i", False, "пропущено (нет базового клиента)")
            return
        try:
            c = self.chan_client
            c.send(f"MODE {self.chan_name} +i")
            lines = c.read_all(1.0)
            applied = any("+i" in l or "MODE" in l for l in lines)
            self.record(
                "MODE +i (invite-only) применяется и подтверждается",
                applied,
                "получено: " + " | ".join(lines) if lines else "нет ответа",
            )

            # Новый клиент пытается зайти без приглашения — должен получить 473
            c4 = self.new_client()
            nick4 = f"noninv_{random_str()}"
            c4.register(nick4, "user4", self.password)
            c4.send(f"JOIN {self.chan_name}")
            lines4 = c4.read_all(1.0)
            blocked = has_code(lines4, "473")  # ERR_INVITEONLYCHAN
            self.record(
                "Без приглашения JOIN в +i канал отклоняется (473)",
                blocked,
                "получено: " + " | ".join(lines4) if lines4 else "нет ответа",
            )

            # Приглашаем и пробуем снова
            c.send(f"INVITE {nick4} {self.chan_name}")
            invite_lines = c.read_all(1.0)
            invited_notice = c4.read_all(1.0)
            c4.send(f"JOIN {self.chan_name}")
            lines4b = c4.read_all(1.0)
            joined_after_invite = has_code(lines4b, "353") or any(
                "JOIN" in l for l in lines4b
            )
            self.record(
                "После INVITE клиент может зайти в invite-only канал",
                joined_after_invite,
                "получено: " + " | ".join(lines4b) if lines4b else "нет ответа",
            )

            c.send(f"MODE {self.chan_name} -i")
            c.read_all(0.5)
            c4.close()
        except Exception as e:
            self.record("MODE +i / INVITE", False, str(e))

    def test_mode_key(self):
        if not getattr(self, "chan_client", None):
            self.record("MODE +k (пароль канала)", False, "пропущено")
            return
        try:
            c = self.chan_client
            key = "secret123"
            c.send(f"MODE {self.chan_name} +k {key}")
            c.read_all(0.5)

            c5 = self.new_client()
            nick5 = f"nokey_{random_str()}"
            c5.register(nick5, "user5", self.password)
            c5.send(f"JOIN {self.chan_name}")
            lines_no_key = c5.read_all(1.0)
            rejected = has_code(lines_no_key, "475")  # ERR_BADCHANNELKEY

            c5.send(f"JOIN {self.chan_name} {key}")
            lines_with_key = c5.read_all(1.0)
            accepted = any("JOIN" in l for l in lines_with_key) or has_code(
                lines_with_key, "353"
            )

            self.record(
                "JOIN без ключа в +k канал отклоняется (475)",
                rejected,
                "получено: " + " | ".join(lines_no_key) if lines_no_key else "нет ответа",
            )
            self.record(
                "JOIN с верным ключом проходит",
                accepted,
                "получено: " + " | ".join(lines_with_key) if lines_with_key else "нет ответа",
            )

            c.send(f"MODE {self.chan_name} -k {key}")
            c.read_all(0.5)
            c5.close()
        except Exception as e:
            self.record("MODE +k", False, str(e))

    # ---------------- PRIVMSG ----------------

    def test_privmsg_user_to_user(self):
        section("Приватные сообщения (PRIVMSG)")
        try:
            c1 = self.new_client()
            c2 = self.new_client()
            n1, n2 = f"pm1_{random_str()}", f"pm2_{random_str()}"
            c1.register(n1, "u1", self.password)
            c2.register(n2, "u2", self.password)

            msg = f"hello_{random_str()}"
            c1.send(f"PRIVMSG {n2} :{msg}")
            reply = c2.expect(lambda l: msg in l)
            self.record(
                "PRIVMSG между двумя пользователями доставляется",
                reply is not None,
                reply or "сообщение не получено",
            )
            c1.close()
            c2.close()
        except Exception as e:
            self.record("PRIVMSG user->user", False, str(e))

    def test_privmsg_to_channel(self):
        if not getattr(self, "chan_client", None):
            self.record("PRIVMSG в канал", False, "пропущено")
            return
        try:
            c_listener = self.new_client()
            nick_l = f"listener_{random_str()}"
            c_listener.register(nick_l, "ul", self.password)
            c_listener.send(f"JOIN {self.chan_name}")
            c_listener.read_all(1.0)

            msg = f"chanmsg_{random_str()}"
            self.chan_client.send(f"PRIVMSG {self.chan_name} :{msg}")
            reply = c_listener.expect(lambda l: msg in l, duration=2.0)
            self.record(
                "PRIVMSG в канал доставляется другим участникам",
                reply is not None,
                reply or "сообщение не получено слушателем",
            )
            c_listener.close()
        except Exception as e:
            self.record("PRIVMSG в канал", False, str(e))

    def test_privmsg_nonexistent_target(self):
        try:
            c = self.new_client()
            c.register(f"ghost_{random_str()}", "u", self.password)
            c.send(f"PRIVMSG no_such_user_{random_str()} :hi")
            lines = c.read_all(1.0)
            err = has_code(lines, "401")  # ERR_NOSUCHNICK
            self.record(
                "PRIVMSG несуществующему нику даёт 401",
                err,
                "получено: " + " | ".join(lines) if lines else "нет ответа",
            )
            c.close()
        except Exception as e:
            self.record("PRIVMSG несуществующему нику", False, str(e))

    # ---------------- Устойчивость / edge cases ----------------

    def test_unknown_command(self):
        section("Устойчивость сервера")
        try:
            c = self.new_client()
            c.register(f"edge_{random_str()}", "u", self.password)
            c.send("FOOBARBAZ arg1 arg2")
            lines = c.read_all(1.0)
            err = has_code(lines, "421")  # ERR_UNKNOWNCOMMAND
            still_alive = True
            try:
                c.send("PING :still_here")
                pong = c.expect(lambda l: "PONG" in l.upper() or "still_here" in l)
                still_alive = pong is not None
            except OSError:
                still_alive = False
            self.record(
                "Неизвестная команда не роняет сервер (421 или игнор)",
                still_alive,
                ("получен 421; " if err else "421 не получен; ")
                + ("сервер жив" if still_alive else "сервер перестал отвечать"),
            )
            c.close()
        except Exception as e:
            self.record("Неизвестная команда", False, str(e))

    def test_partial_command_split_over_packets(self):
        """Проверяет, что сервер корректно склеивает команду, если она пришла по частям."""
        try:
            c = self.new_client()
            nick = f"split_{random_str()}"
            # Отправляем PASS/NICK/USER по кусочкам, имитируя фрагментацию TCP
            c.sock.sendall(f"PASS {self.password}\r\nNIC".encode())
            time.sleep(0.05)
            c.sock.sendall(f"K {nick}\r\nUSER u 0 * :".encode())
            time.sleep(0.05)
            c.sock.sendall(b"real\r\n")
            welcome = c.expect(lambda l: " 001 " in l)
            self.record(
                "Команда, разбитая на несколько TCP-пакетов, обрабатывается корректно",
                welcome is not None,
                welcome or "001 не получен: " + " | ".join(c.last_batch),
            )
            c.close()
        except Exception as e:
            self.record("Фрагментация TCP-пакетов", False, str(e))

    def test_quit_notifies_channel(self):
        if not getattr(self, "chan_client", None):
            self.record("QUIT уведомляет канал", False, "пропущено")
            return
        try:
            c_watcher = self.new_client()
            nick_w = f"watcher_{random_str()}"
            c_watcher.register(nick_w, "uw", self.password)
            c_watcher.send(f"JOIN {self.chan_name}")
            c_watcher.read_all(1.0)

            c_leaver = self.new_client()
            nick_leaver = f"leaver_{random_str()}"
            c_leaver.register(nick_leaver, "ul2", self.password)
            c_leaver.send(f"JOIN {self.chan_name}")
            c_leaver.read_all(1.0)

            c_leaver.send("QUIT :goodbye")
            time.sleep(0.2)
            lines = c_watcher.read_all(1.5)
            notified = any(nick_leaver in l and "QUIT" in l for l in lines)
            self.record(
                "QUIT уведомляет остальных участников канала",
                notified,
                "получено: " + " | ".join(lines) if lines else "нет уведомления",
            )
            c_leaver.close()
            c_watcher.close()
        except Exception as e:
            self.record("QUIT уведомление", False, str(e))

    def cleanup(self):
        for attr in ("chan_client", "chan_client2", "client_main"):
            c = getattr(self, attr, None)
            if c:
                c.close()

    def run_all(self):
        self.test_tcp_connect()
        self.test_wrong_password_rejected()
        self.test_registration_success()
        self.test_nick_collision()
        self.test_ping_pong()

        self.test_join_creates_channel()
        self.test_second_client_join_sees_first()
        self.test_topic()
        self.test_part()
        self.test_kick()

        self.test_mode_invite_only()
        self.test_mode_key()

        self.test_privmsg_user_to_user()
        self.test_privmsg_to_channel()
        self.test_privmsg_nonexistent_target()

        self.test_unknown_command()
        self.test_partial_command_split_over_packets()
        self.test_quit_notifies_channel()

        self.cleanup()


# ---------------------------------------------------------------------------
# Нагрузочный тест
# ---------------------------------------------------------------------------

def stress_test(host, port, password, n_clients=50):
    section(f"Нагрузочный тест ({n_clients} параллельных клиентов)")
    results = {"connected": 0, "registered": 0, "errors": []}
    lock = threading.Lock()

    def worker(i):
        try:
            c = IRCClient(host, port, timeout=5.0)
            with lock:
                results["connected"] += 1
            nick = f"stress{i}_{random_str(4)}"
            welcome = c.register(nick, f"user{i}", password)
            if welcome is not None:
                with lock:
                    results["registered"] += 1
            c.send(f"JOIN #stress")
            c.read_all(0.5)
            c.send(f"PRIVMSG #stress :ping from {nick}")
            c.read_all(0.3)
            c.close()
        except Exception as e:
            with lock:
                results["errors"].append(f"client {i}: {e}")

    threads = [threading.Thread(target=worker, args=(i,)) for i in range(n_clients)]
    start = time.time()
    for t in threads:
        t.start()
    for t in threads:
        t.join(timeout=15)
    elapsed = time.time() - start

    info(f"Подключилось: {results['connected']}/{n_clients}")
    info(f"Успешно зарегистрировалось: {results['registered']}/{n_clients}")
    info(f"Заняло времени: {elapsed:.2f}с")
    if results["errors"]:
        fail(f"Ошибок: {len(results['errors'])} (первые 5 показаны)")
        for e in results["errors"][:5]:
            print(f"      {e}")
    passed = results["registered"] >= n_clients * 0.9  # допускаем 10% погрешности
    if passed:
        ok("Сервер выдержал нагрузку без массовых сбоев")
    else:
        fail("Сервер не справился с нагрузкой (слишком много неудачных регистраций)")
    return passed


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

def main():
    if len(sys.argv) < 3:
        print("Использование: python3 test_ircserv.py <port> <password> [host] [stress_clients]")
        print("Пример:        python3 test_ircserv.py 6667 mypassword 127.0.0.1 50")
        sys.exit(1)

    port = int(sys.argv[1])
    password = sys.argv[2]
    host = sys.argv[3] if len(sys.argv) > 3 else "127.0.0.1"
    n_stress = int(sys.argv[4]) if len(sys.argv) > 4 else 50

    print(f"{COLOR_BOLD}Тестирую FT_IRC сервер на {host}:{port}{COLOR_RESET}")

    try:
        probe = socket.create_connection((host, port), timeout=CONNECT_TIMEOUT)
        probe.close()
    except OSError as e:
        print(f"{COLOR_RED}Не удаётся подключиться к {host}:{port} — {e}{COLOR_RESET}")
        print("Убедись, что сервер запущен: ./ircserv <port> <password>")
        sys.exit(1)

    suite = TestSuite(host, port, password)
    suite.run_all()

    stress_passed = stress_test(host, port, password, n_stress)

    # ---------------- Итог ----------------
    section("ИТОГ")
    total = len(suite.results) + 1
    passed = sum(1 for _, p in suite.results if p) + (1 if stress_passed else 0)
    for name, p in suite.results:
        pass  # уже напечатано по ходу
    print(f"\n{COLOR_BOLD}Пройдено: {passed}/{total}{COLOR_RESET}")
    if passed == total:
        print(f"{COLOR_GREEN}{COLOR_BOLD}Все тесты пройдены ✔{COLOR_RESET}")
    else:
        print(f"{COLOR_YELLOW}{COLOR_BOLD}Есть проваленные тесты — смотри [FAIL] выше.{COLOR_RESET}")
        failed_names = [n for n, p in suite.results if not p]
        if not stress_passed:
            failed_names.append("Нагрузочный тест")
        for n in failed_names:
            print(f"   - {n}")

    sys.exit(0 if passed == total else 1)


if __name__ == "__main__":
    main()
