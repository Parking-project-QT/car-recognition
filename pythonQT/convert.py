import tensorflow as tf
import tf2onnx

# ==========================================
# 1. H5 모델 로드
# ==========================================
model = tf.keras.models.load_model("mnist_model.h5")

print("H5 모델 로딩 완료")
print("Input shape :", model.input_shape)
print("Output shape :", model.output_shape)


# ==========================================
# 2. 추론 함수 생성
# ==========================================
@tf.function(
    input_signature=[
        tf.TensorSpec(
            shape=[None, 28, 28, 1],
            dtype=tf.float32,
            name="input"
        )
    ]
)
def inference(x):
    return {
        "output": model(x, training=False)
    }


# ==========================================
# 3. ConcreteFunction 생성
# ==========================================
concrete_func = inference.get_concrete_function()


# ==========================================
# 4. TensorFlow Graph → ONNX
# ==========================================
model_proto, _ = tf2onnx.convert.from_function(
    inference,
    input_signature=[
        tf.TensorSpec(
            shape=[None, 28, 28, 1],
            dtype=tf.float32,
            name="input"
        )
    ],
    opset=13,
    output_path="model.onnx"
)


print("model.onnx 변환 완료")