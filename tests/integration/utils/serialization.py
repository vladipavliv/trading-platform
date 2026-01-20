import flatbuffers
import struct

from hft.serialization.gen.fbs.domain import (
    LoginRequest,
    LoginResponse,
    TokenBindRequest,
    Message,
    Order,
    OrderStatus,
    TickerPrice,
    MessageUnion,
)

def create_login_request(username: str, password: str) -> bytes:
    builder = flatbuffers.Builder(1024)

    username_offset = builder.CreateString(username)
    password_offset = builder.CreateString(password)

    LoginRequest.LoginRequestStart(builder)
    LoginRequest.LoginRequestAddName(builder, username_offset)
    LoginRequest.LoginRequestAddPassword(builder, password_offset)
    login_req_offset = LoginRequest.LoginRequestEnd(builder)

    Message.MessageStart(builder)
    Message.MessageAddMessageType(builder, MessageUnion.MessageUnion.LoginRequest)
    Message.MessageAddMessage(builder, login_req_offset)
    message_offset = Message.MessageEnd(builder)

    builder.Finish(message_offset)
    return frame(bytes(builder.Output()))

def create_token_bind_request(token: int) -> bytes:
    builder = flatbuffers.Builder(1024)

    TokenBindRequest.TokenBindRequestStart(builder)
    TokenBindRequest.TokenBindRequestAddToken(builder, token)

    token_bind_req_offset = TokenBindRequest.TokenBindRequestEnd(builder)

    Message.MessageStart(builder)
    Message.MessageAddMessageType(builder, MessageUnion.MessageUnion.TokenBindRequest)
    Message.MessageAddMessage(builder, token_bind_req_offset)
    message_offset = Message.MessageEnd(builder)

    builder.Finish(message_offset)
    return frame(bytes(builder.Output()))

def create_order_message(
    order_id: int,
    created: int,
    ticker: str,
    quantity: int,
    price: float,
    action: int
) -> bytes:
    builder = flatbuffers.Builder(1024)

    ticker_offset = builder.CreateString(ticker)

    Order.OrderStart(builder)
    Order.OrderAddId(builder, order_id)
    Order.OrderAddTicker(builder, ticker_offset)
    Order.OrderAddQuantity(builder, quantity)
    Order.OrderAddPrice(builder, price)
    Order.OrderAddAction(builder, action)
    order_offset = Order.OrderEnd(builder)

    Message.MessageStart(builder)
    Message.MessageAddMessageType(builder, MessageUnion.MessageUnion.Order)
    Message.MessageAddMessage(builder, order_offset)
    message_offset = Message.MessageEnd(builder)

    builder.Finish(message_offset)
    return frame(bytes(builder.Output()))

def parse_message(buf: bytes):
    msg = Message.Message.GetRootAsMessage(buf, 0)
    msg_type = msg.MessageType()

    if msg_type == MessageUnion.MessageUnion.LoginRequest:
        login_req = LoginRequest.LoginRequest()
        login_req.Init(msg.Message().Bytes, msg.Message().Pos)
        return login_req

    elif msg_type == MessageUnion.MessageUnion.LoginResponse:
        login_resp = LoginResponse.LoginResponse()
        login_resp.Init(msg.Message().Bytes, msg.Message().Pos)
        return login_resp

    elif msg_type == MessageUnion.MessageUnion.TokenBindRequest:
        token_req = TokenBindRequest.TokenBindRequest()
        token_req.Init(msg.Message().Bytes, msg.Message().Pos)
        return token_req

    elif msg_type == MessageUnion.MessageUnion.Order:
        order = Order.Order()
        order.Init(msg.Message().Bytes, msg.Message().Pos)
        return order

    elif msg_type == MessageUnion.MessageUnion.OrderStatus:
        order_status = OrderStatus.OrderStatus()
        order_status.Init(msg.Message().Bytes, msg.Message().Pos)
        return order_status

    elif msg_type == MessageUnion.MessageUnion.TickerPrice:
        ticker_price = TickerPrice.TickerPrice()
        ticker_price.Init(msg.Message().Bytes, msg.Message().Pos)
        return ticker_price

    else:
        raise ValueError(f"Unknown message type: {msg_type}")

def frame(message: bytes) -> bytes:
    size = struct.pack('<H', len(message))
    return size + message