"""
notes on method:

- create regular slew follower
- remove clamping (so no triangular movement behaviour)
- use gaussian as distribution to see how far to jump

random:
- use EMA to change center of gaussian distribution to emulate the current state of the magnetic material so values outside of it will influence it less.
- use asymmetric curve for soft clipping so we can emulate germanium diode effects (including current leaking)
- values closer to zero crossings should be influenced more because magnetic dipoles can be flipped easier when we're not saturated.
"""
import numpy as np
import matplotlib.pyplot as plt


def nonlin_slew(input_signal, alpha=0.9):
    output_signal = np.zeros_like(input_signal)
    output_signal[0] = input_signal[0]

    ema = input_signal[0]  # Initialize EMA with the first input value

    ema_a = 0.01  # Smoothing factor for EMA

    for i in range(1, len(input_signal)):
        delta = input_signal[i] - output_signal[i - 1]

        # use gaussian as distribution to determine how far to jump
        # jump_distance = np.tanh(alpha * delta)
        # jump_distance = (1 if delta > 0 else -1) * alpha * delta**2
        # jump_distance = np.tanh(delta)*np.exp(18.0*-delta**2) * alpha
        # jump_distance = np.tanh(delta) * np.cos(delta * ema * 2) * alpha * 0.5

        jump_distance = 

        # bruh = alpha * 0.005 + ema * 0.01
        # jump_distance = np.tanh(min(max(delta, -bruh), bruh) * bruh) / np.tanh(bruh)

        # actually use tanh as a more regular slew clipper would

        output_signal[i] = output_signal[i - 1] + jump_distance

        # Update EMA
        ema = ema_a * (np.abs(output_signal[i])) + (1 - ema_a) * ema

    return output_signal


def main():
    # Generate an elegant test wave (increasing amplitude sine wave over time)
    t = np.linspace(0, 4 * np.pi, 5000)
    input_signal = (t / (4 * np.pi)) * np.sin(2 * np.pi * t)
    
    output_signal = nonlin_slew(input_signal)
    
    plt.figure(figsize=(12, 5))
    
    plt.subplot(1, 2, 1)
    plt.plot(t, input_signal, label='Input', alpha=0.7)
    plt.plot(t, output_signal, label='Output', alpha=0.7)
    plt.xlabel('Time')
    plt.ylabel('Amplitude')
    plt.title('Input and Output Signals')
    plt.legend()
    plt.grid(True)
    
    plt.subplot(1, 2, 2)
    plt.plot(input_signal, output_signal, 'b-', linewidth=1.5)
    plt.xlabel('Input')
    plt.ylabel('Output')
    plt.title('Slew Hysteresis Loop')
    plt.grid(True)
    
    plt.tight_layout()
    plt.show()

if __name__ == '__main__':
    main()
