# 0x15 — UML للكود الحالي

المخططات دي بتوصف الكود الموجود في `include/` و`src/` دلوقتي.

## الفهرس

<div dir="rtl">
<ol>
  <li>
    <a href="#1-class-diagrams"><span dir="ltr">Class Diagrams</span></a>
    <ul>
      <li><a href="#11-core-البيانات-الأساسية">1.1 <code dir="ltr">core/</code>: البيانات الأساسية</a></li>
      <li><a href="#12-ports-حدود-التطبيق">1.2 <code dir="ltr">ports/</code>: حدود التطبيق</a></li>
      <li><a href="#13-app-منطق-التطبيق">1.3 <code dir="ltr">app/</code>: منطق التطبيق</a></li>
      <li><a href="#14-protocols-بروتوكولات-الـ-stream">1.4 <code dir="ltr">protocols/</code>: بروتوكولات الـ <span dir="ltr">stream</span></a></li>
      <li><a href="#15-protocols-بروتوكولات-الـ-datagram">1.5 <code dir="ltr">protocols/</code>: بروتوكولات الـ <span dir="ltr">datagram</span></a></li>
      <li><a href="#16-transporttcp-tcpserver">1.6 <code dir="ltr">transport/tcp/</code>: <code dir="ltr">TcpServer</code></a></li>
      <li><a href="#17-transporttcp-tcpsession">1.7 <code dir="ltr">transport/tcp/</code>: <code dir="ltr">TcpSession</code></a></li>
      <li><a href="#18-الصورة-الكاملة">1.8 الصورة الكاملة</a></li>
    </ul>
  </li>
  <li><a href="#2-الملكية-والعمر">الملكية والعمر</a></li>
  <li><a href="#3-sequence-اتصال-جديد"><span dir="ltr">Sequence</span>: اتصال جديد</a></li>
  <li><a href="#4-sequence-رسالة-echo"><span dir="ltr">Sequence</span>: رسالة <span dir="ltr">echo</span></a></li>
  <li><a href="#5-sequence-الإغلاق-بـ-ctrlc"><span dir="ltr">Sequence</span>: الإغلاق بـ <span dir="ltr">Ctrl+C</span></a></li>
  <li><a href="#6-state-دورة-حياة-tcpsession"><span dir="ltr">State</span>: دورة حياة <code dir="ltr">TcpSession</code></a></li>
  <li><a href="#7-package-diagram"><span dir="ltr">Package Diagram</span></a></li>
</ol>
</div>

---

## 1. Class Diagrams

بدل رسمة واحدة كبيرة، هنبني الصورة **طبقة طبقة**، من جوه لبره: نبدأ بالبيانات الأساسية، وبعدين الحدود، وبعدين التطبيق، وبعدين البروتوكولات، وفي الآخر طبقة النقل.

في كل رسمة، الكلاسات الجديدة بتظهر بكل تفاصيلها. أما الكلاسات اللي **اتشرحت قبل كده** فبتظهر كصندوق فاضي فيه الاسم بس، عشان تركز على الجديد.

### إزاي تقرا الأسهم

<div dir="rtl">
<table dir="rtl">
  <thead>
    <tr>
      <th style="text-align: center;">السهم</th>
      <th style="text-align: center;">المعنى</th>
      <th style="text-align: center;">مثال</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code dir="ltr">&lt;|..</code></td>
      <td><span dir="ltr">Realization</span>: الكلاس بيطبّق <span dir="ltr">interface</span></td>
      <td><code dir="ltr">TcpSession</code> بيطبّق <code dir="ltr">IReplyChannel</code></td>
    </tr>
    <tr>
      <td><code dir="ltr">*--</code></td>
      <td><span dir="ltr">Composition</span>: الجزء بيموت مع الكل</td>
      <td><code dir="ltr">TcpSession</code> بيملك <code dir="ltr">IStreamProtocol</code> لوحده</td>
    </tr>
    <tr>
      <td><code dir="ltr">o--</code></td>
      <td><span dir="ltr">Aggregation</span>: ملكية مشتركة أو مرجع</td>
      <td><code dir="ltr">TcpServer</code> و <span dir="ltr">callbacks Asio</span> شايلين <code dir="ltr">shared_ptr&lt;TcpSession&gt;</code></td>
    </tr>
    <tr>
      <td><code dir="ltr">--&gt;</code></td>
      <td><span dir="ltr">Association</span>: بيحتفظ بقيمة من النوع ده</td>
      <td><code dir="ltr">PeerInfo</code> فيه <code dir="ltr">TransportType</code></td>
    </tr>
    <tr>
      <td><code dir="ltr">..&gt;</code></td>
      <td><span dir="ltr">Dependency</span>: بيستخدمه بس</td>
      <td><code dir="ltr">EchoHandler</code> بيستخدم <code dir="ltr">IReplyChannel</code></td>
    </tr>
  </tbody>
</table>

<table dir="rtl">
  <thead>
    <tr>
      <th style="text-align: center;">الرمز جوه الكلاس</th>
      <th style="text-align: center;">المعنى</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code dir="ltr">+</code> / <code dir="ltr">-</code></td>
      <td><span dir="ltr">public</span> / <span dir="ltr">private</span></td>
    </tr>
    <tr>
      <td><code dir="ltr">*</code> بعد الدالة</td>
      <td><span dir="ltr">pure virtual</span></td>
    </tr>
    <tr>
      <td><code dir="ltr">$</code> بعد العضو</td>
      <td><code dir="ltr">static</code></td>
    </tr>
    <tr>
      <td><code dir="ltr">&lt;&lt;interface&gt;&gt;</code></td>
      <td><span dir="ltr">class</span> فيه <span dir="ltr">pure virtual functions</span> بس</td>
    </tr>
    <tr>
      <td><code dir="ltr">&lt;&lt;function&gt;&gt;</code></td>
      <td><code dir="ltr">using X = std::function&lt;...&gt;</code>، مش <span dir="ltr">class</span> حقيقي</td>
    </tr>
  </tbody>
</table>
</div>

---

### 1.1 `core/`: البيانات الأساسية

أبسط طبقة: أنواع بيانات عادية، **مابتعتمدش على أي حاجة**، وكل الطبقات التانية بتستخدمها.

```mermaid
classDiagram
    direction LR

    class Message {
        +std::string payload
    }
    class PeerInfo {
        +TransportType transport
        +std::string address
        +uint16_t port
    }
    class TransportType {
        <<enumeration>>
        Tcp
        Udp
    }

    PeerInfo --> TransportType
```

<div dir="rtl">
<table dir="rtl">
  <thead>
    <tr>
      <th style="text-align: center;">النوع</th>
      <th style="text-align: center;">بيمثل إيه</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code dir="ltr">Message</code></td>
      <td>وحدة البيانات اللي التطبيق بيشوفها. فيها <code dir="ltr">payload</code> بس.</td>
    </tr>
    <tr>
      <td><code dir="ltr">PeerInfo</code></td>
      <td>مين اللي على الطرف التاني: العنوان والبورت ونوع النقل.</td>
    </tr>
    <tr>
      <td><code dir="ltr">TransportType</code></td>
      <td><code dir="ltr">Tcp</code> أو <code dir="ltr">Udp</code>.</td>
    </tr>
  </tbody>
</table>
</div>

---

### 1.2 `ports/`: حدود التطبيق

ال `ports/` هي **العقود** اللي بتحدد إزاي التطبيق بيتكلم مع العالم الخارجي. فيه 3 اتجاهات:

- **من النقل للتطبيق:** `InboundMessageHandler` و`ConnectionOpenedHandler`.
- **من التطبيق للنقل:** `IReplyChannel`.
- **من `main.cpp` للنقل:** `IMessageEndpoint`، عشان يشغّل السيرفر ويوقفه.

```mermaid
classDiagram
    direction LR

    class InboundMessageHandler {
        <<function>>
        void(const InboundMessage&)
    }
    class ConnectionOpenedHandler {
        <<function>>
        void(const PeerInfo&, shared_ptr~IReplyChannel~)
    }
    class InboundMessage {
        +Message message
        +PeerInfo peer
        +shared_ptr~IReplyChannel~ reply_channel
    }
    class IReplyChannel {
        <<interface>>
        +send(Message message)* void
    }
    class IMessageEndpoint {
        <<interface>>
        +start(InboundMessageHandler on_message, ConnectionOpenedHandler on_open)* void
        +stop()* void
    }
    class Message
    class PeerInfo

    IMessageEndpoint ..> InboundMessageHandler : takes
    IMessageEndpoint ..> ConnectionOpenedHandler : takes
    InboundMessageHandler ..> InboundMessage : receives
    ConnectionOpenedHandler ..> PeerInfo : receives
    ConnectionOpenedHandler ..> IReplyChannel : receives
    InboundMessage *-- Message
    InboundMessage *-- PeerInfo
    InboundMessage o-- IReplyChannel : reply_channel
    IReplyChannel ..> Message : sends
```

<div dir="rtl">
<table dir="rtl">
  <thead>
    <tr>
      <th style="text-align: center;">العقد</th>
      <th style="text-align: center;">الاتجاه</th>
      <th style="text-align: center;">مين بيناديه</th>
      <th style="text-align: center;">مين بينفذه</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code dir="ltr">IMessageEndpoint</code></td>
      <td>من <code dir="ltr">main.cpp</code> للنقل</td>
      <td><code dir="ltr">main.cpp</code></td>
      <td><code dir="ltr">TcpServer</code></td>
    </tr>
    <tr>
      <td><code dir="ltr">ConnectionOpenedHandler</code></td>
      <td>من النقل للتطبيق</td>
      <td><code dir="ltr">TcpSession</code></td>
      <td><span dir="ltr">lambda</span> في <code dir="ltr">main.cpp</code> بتنادي <code dir="ltr">EchoHandler::on_open</code></td>
    </tr>
    <tr>
      <td><code dir="ltr">InboundMessageHandler</code></td>
      <td>من النقل للتطبيق</td>
      <td><code dir="ltr">TcpSession</code></td>
      <td><span dir="ltr">lambda</span> في <code dir="ltr">main.cpp</code> بتنادي <code dir="ltr">EchoHandler::on_message</code></td>
    </tr>
    <tr>
      <td><code dir="ltr">IReplyChannel</code></td>
      <td>من التطبيق للنقل</td>
      <td><code dir="ltr">EchoHandler</code></td>
      <td><code dir="ltr">TcpSession</code></td>
    </tr>
  </tbody>
</table>
</div>

**لاحظ:** `InboundMessage` شايل `reply_channel` جواه، فالتطبيق بيستلم الرسالة **ومعاها** الطريقة اللي يرد بيها، من غير ما يعرف هي رايحة فين.

---

### 1.3 `app/`: منطق التطبيق

```mermaid
classDiagram
    direction LR

    class EchoHandler {
        +on_open(PeerInfo peer, shared_ptr~IReplyChannel~ channel) void
        +on_message(InboundMessage inbound) void
    }
    class PeerInfo
    class InboundMessage
    class IReplyChannel {
        <<interface>>
    }

    EchoHandler ..> PeerInfo : reads
    EchoHandler ..> InboundMessage : receives
    EchoHandler ..> IReplyChannel : send()
```

<div dir="rtl">
<table dir="rtl">
  <thead>
    <tr>
      <th style="text-align: center;">الدالة</th>
      <th style="text-align: center;">بتعمل إيه</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code dir="ltr">on_open()</code></td>
      <td>بتبعت رسالتين الترحيب أول ما الاتصال يبقى جاهز.</td>
    </tr>
    <tr>
      <td><code dir="ltr">on_message()</code></td>
      <td>بترجّع نفس الرسالة عن طريق <code dir="ltr">reply_channel-&gt;send()</code>.</td>
    </tr>
  </tbody>
</table>
</div>

**لاحظ:**

<div dir="rtl">
<ul>
  <li>
    <code dir="ltr">EchoHandler</code> <strong>مابيعرفش</strong> <code dir="ltr">TcpSession</code>
    ولا <span dir="ltr">Asio</span> ولا أي بروتوكول. كل اللي يعرفه هو أنواع
    <code dir="ltr">ports/</code> و<code dir="ltr">core/</code>.
  </li>
  <li>
    <strong>مابيورثش من أي <span dir="ltr">interface</span>:</strong>
    <code dir="ltr">main.cpp</code> بيربطه بالسيرفر عن طريق <span dir="ltr">lambdas</span>،
    فهو مش محتاج يعرف <code dir="ltr">IMessageEndpoint</code> ولا <code dir="ltr">TcpServer</code>.
  </li>
</ul>
</div>

---

### 1.4 `protocols/`: بروتوكولات الـ stream

البروتوكول هو اللي **بيفهم البايتات**: بياخد بايتات خام ويطلّع رسائل، وبياخد رسالة ويطلّع بايتات. وهو Sans-I/O، يعني مابيعرفش socket ولا Asio.

```mermaid
classDiagram
    direction TB

    class StreamProtocolFactory {
        <<function>>
        unique_ptr~IStreamProtocol~()
    }
    class IStreamProtocol {
        <<interface>>
        +on_bytes(data, size)* ProtocolEvents
        +encode(Message message)* std::string
        +is_open()* bool
    }
    class RawStreamProtocol {
        +on_bytes(data, size) ProtocolEvents
        +encode(Message message) std::string
        +is_open() bool
    }
    class ProtocolEvents {
        +vector~Message~ messages
        +std::string outbound
        +bool close_after_flush
        +bool error
    }
    class Message

    StreamProtocolFactory ..> IStreamProtocol : creates
    IStreamProtocol <|.. RawStreamProtocol
    IStreamProtocol ..> ProtocolEvents : returns
    IStreamProtocol ..> Message : encodes
    ProtocolEvents *-- "0..*" Message
```

<div dir="rtl">
<table dir="rtl">
  <thead>
    <tr>
      <th style="text-align: center;">الدالة</th>
      <th style="text-align: center;">الدخل</th>
      <th style="text-align: center;">الخرج</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code dir="ltr">on_bytes()</code></td>
      <td>بايتات خام من الـ <span dir="ltr">socket</span></td>
      <td><code dir="ltr">ProtocolEvents</code>: رسائل كاملة، وبايتات لازم تتبعت، وقرار الإغلاق</td>
    </tr>
    <tr>
      <td><code dir="ltr">encode()</code></td>
      <td><code dir="ltr">Message</code> من التطبيق</td>
      <td>بايتات جاهزة للسلك</td>
    </tr>
    <tr>
      <td><code dir="ltr">is_open()</code></td>
      <td>—</td>
      <td><code dir="ltr">false</code> لحد ما الـ <span dir="ltr">handshake</span> يخلص</td>
    </tr>
  </tbody>
</table>
</div>

**لاحظ:**

<div dir="rtl">
<ul>
  <li>
    <code dir="ltr">RawStreamProtocol</code> هو الـ <span dir="ltr">implementation</span> الوحيد دلوقتي،
    وبيعمل نفس سلوك <span dir="ltr">0x14</span>: كل <span dir="ltr">chunk</span> رسالة،
    والـ <span dir="ltr">payload</span> بيتبعت زي ما هو.
  </li>
  <li>
    <code dir="ltr">StreamProtocolFactory</code> موجود عشان
    <strong>كل اتصال ياخد <span dir="ltr">instance</span> لوحده</strong>،
    لأن البروتوكولات اللي فيها <span dir="ltr">framing</span> بيبقى ليها
    <span dir="ltr">state</span> خاص بكل اتصال.
  </li>
</ul>
</div>

---

### 1.5 `protocols/`: بروتوكولات الـ datagram

```mermaid
classDiagram
    direction LR

    class IDatagramProtocol {
        <<interface>>
        +decode(data, size)* optional~Message~
        +encode(Message message)* std::string
    }
    class RawDatagramProtocol {
        +decode(data, size) optional~Message~
        +encode(Message message) std::string
    }
    class Message

    IDatagramProtocol <|.. RawDatagramProtocol
    IDatagramProtocol ..> Message : decodes and encodes
```

**الاتنين دول مفيش حد بيستخدمهم دلوقتي**، لأن مفيش `UdpServer` لسه.

---

### 1.6 `transport/tcp/`: `TcpServer`

أول كلاس بيعرف Asio. شغلته إنه **يقبل الاتصالات** ويعمل لكل اتصال `TcpSession` ومعاها بروتوكول جديد من الـ factory.

```mermaid
classDiagram
    direction LR

    class IMessageEndpoint {
        <<interface>>
    }
    class TcpServer {
        -io_context& m_io
        -tcp::acceptor m_acceptor
        -StreamProtocolFactory m_make_protocol
        -InboundMessageHandler m_on_message
        -ConnectionOpenedHandler m_on_open
        -unordered_set~shared_ptr~TcpSession~~ m_sessions
        +TcpServer(io, port, make_protocol)
        +start(on_message, on_open) void
        +stop() void
        +active_sessions_count() size_t
        -accept() void
        -handle_session_close(shared_ptr~TcpSession~ session) void
    }
    class StreamProtocolFactory
    class InboundMessageHandler
    class ConnectionOpenedHandler
    class TcpSession

    IMessageEndpoint <|.. TcpServer
    TcpServer *-- StreamProtocolFactory : m_make_protocol
    TcpServer *-- InboundMessageHandler : m_on_message
    TcpServer *-- ConnectionOpenedHandler : m_on_open
    TcpServer o-- "0..*" TcpSession : m_sessions (shared_ptr)
```

<div dir="rtl">
<table dir="rtl">
  <thead>
    <tr>
      <th style="text-align: center;">العضو</th>
      <th style="text-align: center;">دوره</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code dir="ltr">m_acceptor</code></td>
      <td>بيسمع على البورت وبيقبل الاتصالات</td>
    </tr>
    <tr>
      <td><code dir="ltr">m_make_protocol</code></td>
      <td>بيعمل بروتوكول جديد لكل اتصال</td>
    </tr>
    <tr>
      <td><code dir="ltr">m_on_message</code> و<code dir="ltr">m_on_open</code></td>
      <td>بيتخزنوا في <code dir="ltr">start()</code>، وبيتنسخوا لكل جلسة جديدة</td>
    </tr>
    <tr>
      <td><code dir="ltr">m_sessions</code></td>
      <td>الجلسات الشغالة، عشان <code dir="ltr">stop()</code> يقدر يقفلها كلها</td>
    </tr>
    <tr>
      <td><code dir="ltr">handle_session_close()</code></td>
      <td>الجلسة بتناديها لما تتقفل، عشان تتشال من <code dir="ltr">m_sessions</code></td>
    </tr>
  </tbody>
</table>
</div>

---

### 1.7 `transport/tcp/`: `TcpSession`

أكبر كلاس في المشروع. بيمثل **اتصال واحد مع عميل واحد**.

```mermaid
classDiagram
    direction LR

    class IReplyChannel {
        <<interface>>
    }
    class TcpSession {
        +size_t kReadBufferSize = 1024$
        +size_t kMaxWriteQueueSize = 64$
        +size_t kMaxMessageSize = 64 KiB$
        +size_t kMaxWriteQueueBytes = 1 MiB$
        -tcp::socket m_socket
        -unique_ptr~IStreamProtocol~ m_protocol
        -PeerInfo m_peer
        -array~char, 1024~ m_read_buffer
        -deque~std::string~ m_write_queue
        -size_t m_write_queue_bytes
        -MessageHandler m_on_message
        -ConnectionOpenedHandler m_on_open
        -CloseCallback m_on_close
        -bool m_stopped
        -bool m_open_notified
        -bool m_close_after_flush
        +socket() tcp::socket&
        +start() void
        +stop() void
        +send(Message message) void
        -read() void
        -write() void
        -enqueue_raw(std::string bytes) void
        -notify_open_if_ready() void
        -handle_error(operation, error) void
    }
    class IStreamProtocol {
        <<interface>>
    }
    class PeerInfo
    class InboundMessageHandler
    class ConnectionOpenedHandler

    IReplyChannel <|.. TcpSession
    TcpSession *-- "1" IStreamProtocol : m_protocol (unique_ptr)
    TcpSession *-- PeerInfo : m_peer
    TcpSession ..> InboundMessageHandler : calls
    TcpSession ..> ConnectionOpenedHandler : calls
```

أعضاء الكلاس كتير، فده تقسيمهم حسب المسؤولية:

<div dir="rtl">
<table dir="rtl">
  <thead>
    <tr>
      <th style="text-align: center;">المسؤولية</th>
      <th style="text-align: center;">الأعضاء</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><strong>قراءة وكتابة الـ <span dir="ltr">socket</span></strong></td>
      <td><code dir="ltr">m_socket</code>، <code dir="ltr">m_read_buffer</code>، <code dir="ltr">read()</code>، <code dir="ltr">write()</code></td>
    </tr>
    <tr>
      <td><strong>طابور الكتابة والـ <span dir="ltr">backpressure</span></strong></td>
      <td><code dir="ltr">m_write_queue</code>، <code dir="ltr">m_write_queue_bytes</code>، <code dir="ltr">enqueue_raw()</code>، والـ 4 ثوابت <code dir="ltr">k...</code></td>
    </tr>
    <tr>
      <td><strong>التعامل مع البروتوكول</strong></td>
      <td><code dir="ltr">m_protocol</code>، <code dir="ltr">send()</code>، <code dir="ltr">notify_open_if_ready()</code>، <code dir="ltr">m_open_notified</code>، <code dir="ltr">m_close_after_flush</code></td>
    </tr>
    <tr>
      <td><strong>مناداة التطبيق</strong></td>
      <td><code dir="ltr">m_on_message</code>، <code dir="ltr">m_on_open</code>، <code dir="ltr">m_peer</code></td>
    </tr>
    <tr>
      <td><strong>دورة الحياة</strong></td>
      <td><code dir="ltr">start()</code>، <code dir="ltr">stop()</code>، <code dir="ltr">handle_error()</code>، <code dir="ltr">m_stopped</code>، <code dir="ltr">m_on_close</code></td>
    </tr>
  </tbody>
</table>
</div>

**لاحظ:**

<div dir="rtl">
<ul>
  <li>
    <strong><code dir="ltr">TcpSession</code> بيلعب دورين:</strong>
    هو الـ <span dir="ltr">adapter</span> بتاع الـ <span dir="ltr">socket</span>،
    وهو كمان الـ <code dir="ltr">IReplyChannel</code> اللي التطبيق بيرد عليه.
    عشان كده <code dir="ltr">InboundMessage::reply_channel</code> بيشاور على الجلسة نفسها.
  </li>
  <li>
    <strong>الجلسة بتملك البروتوكول بـ <code dir="ltr">unique_ptr</code></strong>،
    فكل جلسة ليها بروتوكول لوحدها، والبروتوكول بيموت معاها.
  </li>
  <li>
    <strong><code dir="ltr">m_on_close</code></strong>
    هي <span dir="ltr">lambda</span> من <code dir="ltr">TcpServer</code> بتنادي
    <code dir="ltr">handle_session_close()</code>.
    عشان كده مفيش سهم مباشر من <code dir="ltr">TcpSession</code> لـ <code dir="ltr">TcpServer</code>.
  </li>
</ul>
</div>

---

### 1.8 الصورة الكاملة

دلوقتي بعد ما شفنا كل طبقة لوحدها، دي كل الكلاسات مع بعض، **بالأسماء بس**، عشان تشوف إزاي الطبقات بتتوصل ببعض:

```mermaid
classDiagram
    direction TB

    namespace core {
        class Message
        class PeerInfo
        class TransportType
    }

    namespace ports {
        class IReplyChannel
        class IMessageEndpoint
        class InboundMessage
        class InboundMessageHandler
        class ConnectionOpenedHandler
    }

    namespace protocols {
        class IStreamProtocol
        class ProtocolEvents
        class StreamProtocolFactory
        class RawStreamProtocol
        class IDatagramProtocol
        class RawDatagramProtocol
    }

    namespace app {
        class EchoHandler
    }

    namespace transport_tcp {
        class TcpServer
        class TcpSession
    }

    IReplyChannel <|.. TcpSession
    IMessageEndpoint <|.. TcpServer
    IStreamProtocol <|.. RawStreamProtocol
    IDatagramProtocol <|.. RawDatagramProtocol

    TcpServer o-- "0..*" TcpSession
    TcpSession *-- "1" IStreamProtocol
    TcpSession *-- PeerInfo
    TcpServer *-- StreamProtocolFactory

    InboundMessage *-- Message
    InboundMessage *-- PeerInfo
    InboundMessage o-- IReplyChannel
    ProtocolEvents *-- "0..*" Message
    PeerInfo --> TransportType

    StreamProtocolFactory ..> IStreamProtocol : creates
    IStreamProtocol ..> ProtocolEvents : returns
    TcpSession ..> InboundMessageHandler : calls
    TcpSession ..> ConnectionOpenedHandler : calls
    EchoHandler ..> InboundMessage : receives
    EchoHandler ..> IReplyChannel : send()
```

---

## 2. الملكية والعمر

```mermaid
flowchart TD
    Main["main()"] -->|"unique_ptr"| Server["TcpServer"]
    Main -->|"stack object"| Echo["EchoHandler"]
    Server -->|"shared_ptr in m_sessions"| Session["TcpSession"]
    Pending["Pending Asio handlers<br/>(read, write, accept)"] -->|"shared_ptr self"| Session
    Inbound["InboundMessage.reply_channel"] -->|"shared_ptr (temporary)"| Session
    Session -->|"unique_ptr"| Proto["IStreamProtocol"]
    Server -->|"copies of std::function"| Echo
```

<div dir="rtl">
<table dir="rtl">
  <thead>
    <tr>
      <th style="text-align: center;">الـ <span dir="ltr">Object</span></th>
      <th style="text-align: center;">مين بيملكه</th>
      <th style="text-align: center;">بيموت إمتى</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code dir="ltr">TcpServer</code></td>
      <td><code dir="ltr">main()</code> عن طريق <code dir="ltr">unique_ptr</code></td>
      <td>آخر <code dir="ltr">main()</code></td>
    </tr>
    <tr>
      <td><code dir="ltr">EchoHandler</code></td>
      <td><code dir="ltr">main()</code> على الـ <span dir="ltr">stack</span></td>
      <td>آخر <code dir="ltr">main()</code></td>
    </tr>
    <tr>
      <td><code dir="ltr">TcpSession</code></td>
      <td><code dir="ltr">m_sessions</code>، وأي <span dir="ltr">async handler</span> شغال</td>
      <td>لما يتشال من <code dir="ltr">m_sessions</code> ومايبقاش فيه <span dir="ltr">handler</span> شايله</td>
    </tr>
    <tr>
      <td><code dir="ltr">IStreamProtocol</code></td>
      <td><code dir="ltr">TcpSession</code> لوحده</td>
      <td>مع الجلسة</td>
    </tr>
  </tbody>
</table>
</div>

---

## 3. Sequence: اتصال جديد

```mermaid
sequenceDiagram
    autonumber
    participant C as Client
    participant Srv as TcpServer
    participant F as StreamProtocolFactory
    participant S as TcpSession
    participant P as RawStreamProtocol
    participant A as EchoHandler

    Srv->>F: m_make_protocol()
    F-->>Srv: unique_ptr IStreamProtocol
    Srv->>S: make_shared TcpSession(io, protocol, on_message, on_open, on_close)
    Srv->>Srv: async_accept(session->socket())
    C->>Srv: TCP connect (SYN, SYN-ACK, ACK)
    Srv->>Srv: m_sessions.insert(session)
    Srv->>S: start()
    S->>S: read remote_endpoint into m_peer
    S->>S: notify_open_if_ready()
    S->>P: is_open()
    P-->>S: true
    S->>A: on_open(peer, self)
    A->>S: send("Welcome ...")
    S->>P: encode(message)
    P-->>S: bytes
    S->>S: enqueue_raw(bytes) then write()
    A->>S: send("Type anything ...")
    S->>S: enqueue_raw(bytes) and wait for current write
    S->>S: read() starts async_read_some
    Srv->>Srv: accept() again for the next client
```

---

## 4. Sequence: رسالة echo

```mermaid
sequenceDiagram
    autonumber
    participant C as Client
    participant S as TcpSession
    participant P as RawStreamProtocol
    participant A as EchoHandler

    C->>S: bytes "hello"
    S->>P: on_bytes(buffer, n)
    P-->>S: ProtocolEvents with messages = [hello]
    Note over S: outbound is empty, so nothing extra to send
    S->>S: notify_open_if_ready() does nothing (already notified)
    S->>A: m_on_message(InboundMessage)
    A->>S: reply_channel->send(message)
    S->>P: encode(message)
    P-->>S: "hello"
    S->>S: enqueue_raw then async_write
    S-->>C: bytes "hello"
    Note over S: error and close_after_flush are false
    S->>S: read() again
```

---

## 5. Sequence: الإغلاق بـ Ctrl+C

```mermaid
sequenceDiagram
    autonumber
    participant OS as OS signal
    participant M as main()
    participant Srv as TcpServer
    participant S as TcpSession
    participant IO as io_context

    OS->>M: SIGINT
    M->>Srv: stop()
    Srv->>Srv: m_acceptor.close()
    loop every session in a copy of m_sessions
        Srv->>S: stop()
        S->>S: m_stopped = true, shutdown and close socket
        S->>Srv: on_close(self) then handle_session_close()
    end
    Srv->>Srv: m_sessions.clear()
    IO->>Srv: accept handler with operation_aborted
    IO->>S: read handler with operation_aborted
    Note over S: the last shared_ptr is released, so the destructor runs
    IO-->>M: io.run() returns because no work is left
```

---

## 6. State: دورة حياة `TcpSession`

```mermaid
stateDiagram-v2
    [*] --> Created: make_shared in accept()
    Created --> WaitingHandshake: start() while is_open() is false
    Created --> Open: start() while is_open() is true
    WaitingHandshake --> Open: on_bytes() completes the handshake
    Open --> Open: read, deliver messages, write
    Open --> Draining: close_after_flush
    Draining --> Stopped: write queue is empty
    WaitingHandshake --> Stopped: error, EOF, or stop()
    Open --> Stopped: error, EOF, backpressure, or stop()
    Stopped --> [*]: last shared_ptr released
```

<div dir="rtl">
<table dir="rtl">
  <thead>
    <tr>
      <th style="text-align: center;">الحالة</th>
      <th style="text-align: center;">الفلاج المقابل في الكود</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><span dir="ltr">WaitingHandshake</span></td>
      <td><code dir="ltr">m_open_notified == false</code> و<code dir="ltr">is_open() == false</code> (مش بيحصل مع <span dir="ltr">Raw</span>)</td>
    </tr>
    <tr>
      <td><span dir="ltr">Open</span></td>
      <td><code dir="ltr">m_open_notified == true</code></td>
    </tr>
    <tr>
      <td><span dir="ltr">Draining</span></td>
      <td><code dir="ltr">m_close_after_flush == true</code></td>
    </tr>
    <tr>
      <td><span dir="ltr">Stopped</span></td>
      <td><code dir="ltr">m_stopped == true</code></td>
    </tr>
  </tbody>
</table>
</div>

---

## 7. Package Diagram

```mermaid
flowchart TD
    Main["src/main.cpp<br/>(Composition Root)"]
    App["app/"]
    Ports["ports/"]
    Protocols["protocols/<br/>(no Asio)"]
    Transport["transport/tcp/<br/>(Asio)"]
    Core["core/"]
    Common["common/debug.hpp"]

    Main --> App
    Main --> Protocols
    Main --> Transport
    App --> Ports
    Transport --> Ports
    Transport --> Protocols
    Ports --> Core
    Protocols --> Core
    App -.-> Common
    Transport -.-> Common
```

**القاعدة:** الأسهم بتشاور ناحية `core/` بس. `app/` و`protocols/` مابيعرفوش بعض، و`transport/tcp/` هو الوحيد اللي بيعمل `#include <asio.hpp>`.