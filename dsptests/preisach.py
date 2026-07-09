import numpy as np
import matplotlib.pyplot as plt

"""
notes on method:

- in preisach, we are creating a series of N^2 hysterons that are all summed together to create hysteresis.

2d table version
- in discrete model, a hysteron is a function that is either 1 or -1. when input is moving up, it will switch to 1 when the input is above alpha, and only when moving down, it will switch to -1 when the input is below beta.
- alpha and beta are the two thresholds that define the hysteron. they can be different for each hysteron.
- each hysteron has its own weight (multiplier on output) that is computed according to a distribution
- the distribution can be anything but a gaussian is useful because the sum of the curve under a gaussian is the error function (almost looks like tanh), which is a useful saturation curve
- we represent every single possible hysteron that could have a possible alpha and beta on a 2D grid of NxN called an everatt table
- half of these values are useless because beta can't be below alpha.
- without precomputing values, every hysteron needs to be calculated for every possible value of alpha and beta.
- we precompute a NxN grid of values called the everett map that stores the area under the curve of the distribution from a baseline value up to a specific alpha and beta.
- we maintain a stack of peaks and troughs that are used to compute the current value given to the system by adding and summing areas
- for each read, we do bilinear filtering on output to avoid aliasing with stepped grid values in the table.
- optional oversampling to avoid aliasing with the stepped grid values.

continuous version
- with the continuous version, we assume we have an infinite sum of hysterons.
- instead of precomputing the area of the curve under the distribution in a table, we can get sums from any arbitrary signal peak and trough value if we know the closed form integral of our distribution
- instead of iterating N^2 times we can now compute once.
- we maintain the stack in the same way
"""

def sech(x):
    return np.cosh(x)**(-1)


def hysteron(input_signal, alpha, beta, saturation_point=1.0):
    """
    Simulate a single hysteron effect on the input signal.
    """
    output_signal = np.zeros_like(input_signal)
    # pick randomly whether to start at -1 or +1
    state = saturation_point if np.random.rand() > 0.5 else -saturation_point
    
    for i, x in enumerate(input_signal):
        if x > alpha:
            state = saturation_point
        elif x < beta:
            state = -saturation_point
        output_signal[i] = state
    
    return output_signal

num_hysterons = 801
step_size = 2.0 / (num_hysterons - 1)
sigma = 0.7
g = 0.7

sigmadiv = 1 / (2 * (sigma ** 2))
gdiv = 1 / (2 * (g ** 2))

# 2d table precomputed
density = np.zeros((num_hysterons, num_hysterons))
for i in range(num_hysterons):
    for j in range(num_hysterons):
        alpha = -1.0 + i * step_size
        beta  = -1.0 + j * step_size
        if alpha >= beta:
            # Using your sech distribution logic
            density[i, j] = np.exp(-((alpha + beta)**2) * sigmadiv - ((alpha - beta) ** 2) * gdiv)

# build up values in table using dynamic programming
hysteron_ab_sums_table = np.zeros((num_hysterons, num_hysterons))

for diff in range(num_hysterons):
    for j in range(num_hysterons - diff):
        i = j + diff
        
        T_left   = hysteron_ab_sums_table[i-1, j] if (i-1 >= j) else 0.0
        T_bottom = hysteron_ab_sums_table[i, j+1] if (i >= j+1) else 0.0
        T_diag   = hysteron_ab_sums_table[i-1, j+1] if (i-1 >= j+1) else 0.0
        
        hysteron_ab_sums_table[i, j] = T_left + T_bottom - T_diag + density[i, j]

E_max = hysteron_ab_sums_table[num_hysterons - 1, 0]
if E_max == 0.0:
    E_max = 1.0


def get_interpolated_area(alpha, beta):
    idx_alpha = (alpha + 1.0) * 0.5 * (num_hysterons - 1)
    idx_beta = (beta + 1.0) * 0.5 * (num_hysterons - 1)
    
    idx_alpha = max(0.0, min(float(num_hysterons - 1), idx_alpha))
    idx_beta = max(0.0, min(float(num_hysterons - 1), idx_beta))
    
    i = int(idx_alpha)
    j = int(idx_beta)
    
    frac_a = idx_alpha - i
    frac_b = idx_beta - j
    
    i_next = min(i + 1, num_hysterons - 1)
    j_next = min(j + 1, num_hysterons - 1)
    
    T00 = hysteron_ab_sums_table[i, j]          
    T10 = hysteron_ab_sums_table[i_next, j]      
    T01 = hysteron_ab_sums_table[i, j_next]     
    T11 = hysteron_ab_sums_table[i_next, j_next] 
    
    bottom_mix = T00 * (1.0 - frac_a) + T10 * frac_a
    top_mix = T01 * (1.0 - frac_a) + T11 * frac_a
    
    final_area = bottom_mix * (1.0 - frac_b) + top_mix * frac_b
    
    return final_area


def hysteron_fast_sum(input_signal, saturation_point=1.0):
    """
    2d preisach with history
    """
    output_signal = np.zeros_like(input_signal)
    
    stack_M = []
    stack_m = [-1.0]
    is_rising = True
    
    last_signal = input_signal[0]

    for i, x in enumerate(input_signal):
        now_rising = (x >= last_signal)
        if now_rising != is_rising:
            if is_rising:
                stack_M.append(last_signal)
            else:
                stack_m.append(last_signal)
            is_rising = now_rising

        if is_rising:
            while len(stack_M) > 0 and x >= stack_M[-1]:
                stack_M.pop()
                if len(stack_m) > 1: 
                    stack_m.pop()
        else:
            while len(stack_m) > 1 and x <= stack_m[-1]:
                stack_m.pop()
                if len(stack_M) > 0: 
                    stack_M.pop()
        
        accumulated_area = 0.0
        
        for k in range(len(stack_M)):
            accumulated_area += get_interpolated_area(stack_M[k], stack_m[k])
            if k + 1 < len(stack_m):
                accumulated_area -= get_interpolated_area(stack_M[k], stack_m[k+1])
        
        if is_rising:
            accumulated_area += get_interpolated_area(x, stack_m[-1])
            current_output = -1.0 + (2.0 * accumulated_area / E_max)
        else:
            if len(stack_M) > 0:
                accumulated_area -= get_interpolated_area(stack_M[-1], x)
            else:
                accumulated_area -= get_interpolated_area(1.0, x)
            current_output = -1.0 + (2.0 * accumulated_area / E_max)

        output_signal[i] = current_output * saturation_point
        last_signal = x

    return output_signal

def hysteron_slow_sum(input_signal, saturation_point=1.0):
    """
    manually computes sum of all hysterons for all samples
    """
    output_signal = np.zeros_like(input_signal)
    step_size = 2.0 / (num_hysterons - 1)
    sigma = 0.001 
    g = 0.1 # width of hysteresis
    total_weight = 0.0

    sigmadiv = 1 / (2 * (sigma ** 2))
    gdiv = 1 / (2 * (g ** 2))

    
    for i in range(num_hysterons):
        for j in range(num_hysterons):
            alpha = -1.0 + i * step_size
            beta  = -1.0 + j * step_size
            
            if alpha >= beta:
                # gaussian distribution
                weight = np.exp(-((alpha + beta)**2) * sigmadiv - ((alpha - beta) ** 2) * gdiv)

                total_weight += weight

                hysteron_output = hysteron(input_signal, alpha, beta, saturation_point)
                output_signal += hysteron_output * weight

    # normalize
    if total_weight > 0:
        output_signal /= total_weight
    
    return output_signal

def main():
    t = np.linspace(0, 4 * np.pi, 5000)
    input_signal = (t / (4 * np.pi)) * np.sin(2 * np.pi * t) # sine wave
    
    output_signal = hysteron_fast_sum(input_signal)
    
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
    plt.title('Preisach Hysteresis Loop')
    plt.grid(True)
    
    plt.tight_layout()
    plt.show()

if __name__ == '__main__':
    main()
