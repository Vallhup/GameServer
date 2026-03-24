using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WITH_ServerDataTool.Utility
{
    public enum ValidationSeverity
    {
        Warning,
        Error
    }

    public sealed class ValidationMessage
    {
        public ValidationSeverity Severity { get; set; }
        public string Path { get; set; }
        public string Message { get; set; }

        public override string ToString()
        {
            if (string.IsNullOrEmpty(Path))
                return Message ?? string.Empty;

            return Path + ": " + (Message ?? string.Empty);
        }
    }

    public sealed class ValidationResult
    {
        private readonly List<ValidationMessage> _messages;

        public ValidationResult()
        {
            _messages = new List<ValidationMessage>();
        }

        public IList<ValidationMessage> Messages
        {
            get { return _messages; }
        }

        public bool IsValid
        {
            get
            {
                for (int i = 0; i < _messages.Count; ++i)
                {
                    if (_messages[i].Severity == ValidationSeverity.Error)
                        return false;
                }

                return true;
            }
        }

        public int ErrorCount
        {
            get
            {
                int count = 0;

                for (int i = 0; i < _messages.Count; ++i)
                {
                    if (_messages[i].Severity == ValidationSeverity.Error)
                        ++count;
                }

                return count;
            }
        }

        public int WarningCount
        {
            get
            {
                int count = 0;

                for (int i = 0; i < _messages.Count; ++i)
                {
                    if (_messages[i].Severity == ValidationSeverity.Warning)
                        ++count;
                }

                return count;
            }
        }

        public void AddError(string path, string message)
        {
            _messages.Add(new ValidationMessage
            {
                Severity = ValidationSeverity.Error,
                Path = path,
                Message = message
            });
        }

        public void AddWarning(string path, string message)
        {
            _messages.Add(new ValidationMessage
            {
                Severity = ValidationSeverity.Warning,
                Path = path,
                Message = message
            });
        }

        public void Merge(ValidationResult other)
        {
            if (other == null)
                return;

            for (int i = 0; i < other.Messages.Count; ++i)
                _messages.Add(other.Messages[i]);
        }

        public string ToDisplayString()
        {
            StringBuilder sb = new StringBuilder();

            sb.AppendLine(IsValid
                ? "Validation succeeded."
                : "Validation failed.");

            sb.Append("Errors: ");
            sb.AppendLine(ErrorCount.ToString());

            sb.Append("Warnings: ");
            sb.AppendLine(WarningCount.ToString());

            if (_messages.Count == 0)
                return sb.ToString();

            sb.AppendLine();
            sb.AppendLine("[Messages]");

            int index = 1;
            for (int i = 0; i < _messages.Count; ++i)
            {
                ValidationMessage msg = _messages[i];

                sb.Append(index);
                sb.Append(". ");

                if (msg.Severity == ValidationSeverity.Error)
                    sb.Append("[Error] ");
                else
                    sb.Append("[Warning] ");

                if (string.IsNullOrEmpty(msg.Path) == false)
                {
                    sb.Append(msg.Path);
                    sb.Append(": ");
                }

                sb.AppendLine(msg.Message ?? string.Empty);
                ++index;
            }

            return sb.ToString();
        }
    }
}
