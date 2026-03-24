using System;

namespace WITH_ServerDataTool.Utility
{
    public static class IdEnumUtil
    {
        public static bool IsNone<T>(T value) where T : struct, Enum
        {
            return Convert.ToUInt64(value) == 0UL;
        }

        public static void EnsureDefined<T>(T value, string paramName)
            where T : struct, Enum
        {
            if(!Enum.IsDefined(typeof(T), value))
                throw new ArgumentOutOfRangeException(paramName, value,
                    $"Undefined {typeof(T).Name} value.");
        }
    }
}
