local EasingModes = {}

local Enum = Enum or import("Enum")
local EasingModes = {}

local math_cos = math.cos
local math_sin = math.sin
local math_pi = math.pi

local bounce_out, bounce_inv

EasingModes[Enum.StyleMode.Bounce] = {
    [Enum.EaseMode.Out] = function(t)
        local n1, d1 = 121/16, 11/4
        local t1 = t - 1.5 / d1
        local t2 = t - 2.25 / d1
        local t3 = t - 2.625 / d1
        
        return t < 1 / d1 ? n1 * t * t :
               t < 2 / d1 ? n1 * t1 * t1 + 3/4 :
               t < 2.5 / d1 ? n1 * t2 * t2 + 15/16 :
               n1 * t3 * t3 + 63/64
    end,
    [Enum.EaseMode.In] = function(t)
        return 1 - bounce_out(1 - t)
    end,
    [Enum.EaseMode.InOut] = function(t)
        return t < 0.5 ? bounce_inv(t * 2) * 0.5 : bounce_out(t * 2 - 1) * 0.5 + 0.5
    end
}
bounce_out = EasingModes[Enum.StyleMode.Bounce][Enum.EaseMode.Out]
bounce_inv = EasingModes[Enum.StyleMode.Bounce][Enum.EaseMode.In]

EasingModes[Enum.StyleMode.Cubic] = {
    [Enum.EaseMode.In] = function(t)
        return t * t * t
    end,
    [Enum.EaseMode.Out] = function(t)
        return (t - 1)^3 + 1
		--local d = t - 1
        --return d * d * d + 1
    end,
    [Enum.EaseMode.InOut] = function(t)
        local nt = 2 * t - 2
        return t < 0.5 ? 4 * t * t * t : 0.5 * nt * nt * nt + 1
    end
}

EasingModes[Enum.StyleMode.Exponential] = {
    [Enum.EaseMode.In] = function(t)
        return t == 0 ? 0 : 2^(10 * (t - 1))
    end,
    [Enum.EaseMode.Out] = function(t)
        return t == 1 ? 1 : 1 - 2^(-10 * t)
    end,
    [Enum.EaseMode.InOut] = function(t)
        return t == 0 ? 0 :
               t == 1 ? 1 :
               t < 0.5 ? 2^(20 * t - 10) / 2 :
               (2 - 2^(-20 * t + 10)) / 2
    end
}

EasingModes[Enum.StyleMode.Quad] = {
    [Enum.EaseMode.In] = function(t)
        return t * t
    end,
    [Enum.EaseMode.Out] = function(t)
        return t * (2 - t)
    end,
    [Enum.EaseMode.InOut] = function(t)
        return t < 0.5 ? 2 * t * t : 4 * t - 2 * t * t - 1
    end
}

EasingModes[Enum.StyleMode.Sine] = {
    [Enum.EaseMode.In] = function(t)
        return 1 - math_cos((t * math_pi) / 2)
    end,
    [Enum.EaseMode.Out] = function(t)
        return math_sin((t * math_pi) / 2)
    end,
    [Enum.EaseMode.InOut] = function(t)
        return -(math_cos(math_pi * t) - 1) / 2
    end
}

return EasingModes