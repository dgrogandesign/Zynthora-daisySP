import math

SAMPLES = 2048
FRAMES = 64

def generate_table():
    header_content = []
    header_content.append("#ifndef WAVETABLES_H")
    header_content.append("#define WAVETABLES_H")
    header_content.append("")
    header_content.append(f"const float WAVETABLE_BANK[{FRAMES}][{SAMPLES}] = {{")

    for frame in range(FRAMES):
        # Normalize frame 0.0 to 1.0
        t = frame / (FRAMES - 1)
        
        # Calculate Morph Parameters
        # 1. Additive: Number of Harmonics (1 to 64)
        num_harmonics = 1 + int(t * 30 + (math.sin(t * math.pi) * 20))
        
        # 2. FM: Index (0.0 to 3.0)
        fm_index = t * 2.5 * math.sin(t * math.pi)

        # 3. Formant Shift
        formant_shift = 1.0 + t * 0.5

        samples = []
        for i in range(SAMPLES):
            phase = (i / SAMPLES) * 2.0 * math.pi
            val = 0.0
            
            # --- Synthesis Engine ---
            
            # Layer 1: Additive Saw/Square Morph
            for k in range(1, num_harmonics + 1):
                amp = 1.0 / k
                # Morph even harmonics out for "Square" feel in middle
                if k % 2 == 0:
                    amp *= (1.0 - math.sin(t * math.pi)) 
                
                # Apply Formant Shift to frequency
                val += amp * math.sin(k * phase * formant_shift)

            # Layer 2: FM Warping (Modulator)
            mod = math.sin(phase * 2.0) * fm_index
            val = math.sin(phase + mod + val * 0.2) 
            
            # Normalize approx
            val *= 0.4 
            
            samples.append(f"{val:.6f}f")
        
        header_content.append("    { " + ", ".join(samples) + " },")

    header_content.append("};")
    header_content.append("")
    header_content.append("#endif")
    
    return "\n".join(header_content)

if __name__ == "__main__":
    print(f"Generating {FRAMES} frames of {SAMPLES} samples...")
    content = generate_table()
    with open("src/wavetables.h", "w") as f:
        f.write(content)
    print("Done! Written to ../src/wavetables.h")
